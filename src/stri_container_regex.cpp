/* This file is part of the 'stringi' project.
 * Copyright (c) 2013-2020, Marek Gagolewski <https://www.gagolewski.com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING,
 * BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


#include "stri_stringi.h"
#include "stri_container_regex.h"


/**
 * Default constructor
 *
 */
StriContainerRegexPattern::StriContainerRegexPattern()
    : StriContainerUTF16()
{
    this->lastMatcherIndex = -1;
    this->lastMatcher = NULL;
    //this->opts = 0;
}


/**
 * Construct String Container from R character vector
 * @param rstr R character vector
 * @param nrecycle extend length [vectorization]
 * @param flags regexp flags
 */
StriContainerRegexPattern::StriContainerRegexPattern(SEXP rstr, R_len_t _nrecycle, StriRegexMatcherOptions _opts)
    : StriContainerUTF16(rstr, _nrecycle, true)
{
    this->lastMatcherIndex = -1;
    this->lastMatcher = NULL;
    this->opts = _opts;

    R_len_t n = get_n();
    for (R_len_t i=0; i<n; ++i) {
        if (!isNA(i) && get(i).length() <= 0) {
            Rf_warning(MSG__EMPTY_SEARCH_PATTERN_UNSUPPORTED);
        }
    }
}


/** Copy constructor
 *
 */
StriContainerRegexPattern::StriContainerRegexPattern(StriContainerRegexPattern& container)
    :    StriContainerUTF16((StriContainerUTF16&)container)
{
    this->lastMatcherIndex = -1;
    this->lastMatcher = NULL;
    this->opts = container.opts;
}


StriContainerRegexPattern& StriContainerRegexPattern::operator=(StriContainerRegexPattern& container)
{
    this->~StriContainerRegexPattern();
    (StriContainerUTF16&) (*this) = (StriContainerUTF16&)container;
    this->lastMatcherIndex = -1;
    this->lastMatcher = NULL;
    this->opts = container.opts;
    return *this;
}


/** Destructor
 *
 */
StriContainerRegexPattern::~StriContainerRegexPattern()
{
    if (lastMatcher) {
        delete lastMatcher;
        lastMatcher = NULL;
    }
}


/** The returned matcher shall not be deleted by the user
 *
 * it is assumed that \code{vectorize_next()} is used:
 * for \code{i >= this->n} the last matcher is returned
 *
 * @param i index
 */
RegexMatcher* StriContainerRegexPattern::getMatcher(R_len_t i)
{
    if (lastMatcher) {
        if (this->lastMatcherIndex == (i % n)) {
            return lastMatcher; // reuse
        }
        else {
            delete lastMatcher; // invalidate
            lastMatcher = NULL;
        }
    }

    UErrorCode status = U_ZERO_ERROR;
    lastMatcher = new RegexMatcher(this->get(i), opts.flags, status);

    if (U_FAILURE(status)) {
        if (lastMatcher) delete lastMatcher;
        lastMatcher = NULL;

        const char* context; // to ease debugging, #382
        std::string s;
        if (str[i%n].isBogus())
            context = NULL;
        else {
            str[i%n].toUTF8String(s);
            context = s.c_str();
        }

        throw StriException(status, context);
    }

    if (!lastMatcher) throw StriException(MSG__MEM_ALLOC_ERROR);

    if (opts.stack_limit > 0) {
        lastMatcher->setStackLimit(opts.stack_limit, status);
        STRI__CHECKICUSTATUS_THROW(status, {/* do nothing special on err */})
    }

    if (opts.time_limit > 0) {
        lastMatcher->setTimeLimit(opts.time_limit, status);
        STRI__CHECKICUSTATUS_THROW(status, {/* do nothing special on err */})
    }


    this->lastMatcherIndex = (i % n);

    return lastMatcher;
}


/** Read regex flags from a list
 *
 * may call Rf_error
 *
 * @param opts_regex list
 * @return flags
 *
 * @version 0.1-?? (Marek Gagolewski)
 *
 * @version 0.2-3 (Marek Gagolewski, 2014-05-09)
 *          allow NULL for opts_regex
 *
 * @version 0.3-1 (Marek Gagolewski, 2014-11-05)
 *    Disallow NA options
 *
 * @version 1.1.6 (Marek Gagolewski, 2017-11-10)
 *    PROTECT STRING_ELT(names, i)
 *
 * @version 1.4.7 (Marek Gagolewski, 2020-08-24)
 *    add time_limit and stack_limit
 */
StriRegexMatcherOptions StriContainerRegexPattern::getRegexOptions(SEXP opts_regex)
{
    int32_t stack_limit = 0;
    int32_t time_limit = 0;
    uint32_t flags = 0;
    if (!isNull(opts_regex) && !Rf_isVectorList(opts_regex))
        Rf_error(MSG__ARG_EXPECTED_LIST, "opts_regex"); // error() call allowed here

    R_len_t narg = isNull(opts_regex)?0:LENGTH(opts_regex);

    if (narg > 0) {

        SEXP names = PROTECT(Rf_getAttrib(opts_regex, R_NamesSymbol));
        if (names == R_NilValue || LENGTH(names) != narg)
            Rf_error(MSG__REGEXP_CONFIG_FAILED); // error() call allowed here

        for (R_len_t i=0; i<narg; ++i) {
            if (STRING_ELT(names, i) == NA_STRING)
                Rf_error(MSG__REGEXP_CONFIG_FAILED); // error() call allowed here

            SEXP tmp_arg;
            PROTECT(tmp_arg = STRING_ELT(names, i));
            const char* curname = stri__copy_string_Ralloc(tmp_arg, "curname");  /* this is R_alloc'ed */
            UNPROTECT(1);

            PROTECT(tmp_arg = VECTOR_ELT(opts_regex, i));
            if  (!strcmp(curname, "case_insensitive")) {
                bool val = stri__prepare_arg_logical_1_notNA(tmp_arg, "case_insensitive");
                if (val) flags |= UREGEX_CASE_INSENSITIVE;
            } else if  (!strcmp(curname, "comments")) {
                bool val = stri__prepare_arg_logical_1_notNA(tmp_arg, "comments");
                if (val) flags |= UREGEX_COMMENTS;
            } else if  (!strcmp(curname, "dotall")) {
                bool val = stri__prepare_arg_logical_1_notNA(tmp_arg, "dotall");
                if (val) flags |= UREGEX_DOTALL;
            } else if  (!strcmp(curname, "literal")) {
                bool val = stri__prepare_arg_logical_1_notNA(tmp_arg, "literal");
                if (val) flags |= UREGEX_LITERAL;
            } else if  (!strcmp(curname, "multiline")) {
                bool val = stri__prepare_arg_logical_1_notNA(tmp_arg, "multiline");
                if (val) flags |= UREGEX_MULTILINE;
            } else if  (!strcmp(curname, "unix_lines")) {
                bool val = stri__prepare_arg_logical_1_notNA(tmp_arg, "unix_lines");
                if (val) flags |= UREGEX_UNIX_LINES;
            } else if  (!strcmp(curname, "uword")) {
                bool val = stri__prepare_arg_logical_1_notNA(tmp_arg, "uword");
                if (val) flags |= UREGEX_UWORD;
            } else if  (!strcmp(curname, "error_on_unknown_escapes")) {
                bool val = stri__prepare_arg_logical_1_notNA(tmp_arg, "error_on_unknown_escapes");
                if (val) flags |= UREGEX_ERROR_ON_UNKNOWN_ESCAPES;
            } else if  (!strcmp(curname, "stack_limit")) {
                stack_limit = stri__prepare_arg_integer_1_notNA(tmp_arg, "stack_limit");
            } else if  (!strcmp(curname, "time_limit")) {
                time_limit = stri__prepare_arg_integer_1_notNA(tmp_arg, "time_limit");
            } else {
                Rf_warning(MSG__INCORRECT_REGEX_OPTION, curname);
            }
            UNPROTECT(1);
        }
        UNPROTECT(1); /* names */
    }

    StriRegexMatcherOptions opts;
    opts.flags = flags;
    opts.time_limit = time_limit;
    opts.stack_limit = stack_limit;
    return opts;
}
