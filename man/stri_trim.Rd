% Generated by roxygen2: do not edit by hand
% Please edit documentation in R/trim.R
\name{stri_trim_both}
\alias{stri_trim_both}
\alias{stri_trim}
\alias{stri_trim_left}
\alias{stri_trim_right}
\title{Trim Characters from the Left and/or Right Side of a String}
\usage{
stri_trim_both(str, pattern = "\\\\P{Wspace}")

stri_trim_left(str, pattern = "\\\\P{Wspace}")

stri_trim_right(str, pattern = "\\\\P{Wspace}")

stri_trim(str, side = c("both", "left", "right"), pattern = "\\\\P{Wspace}")
}
\arguments{
\item{str}{a character vector of strings to be trimmed}

\item{pattern}{a single pattern, specifying character
classes that should be preserved (see \link{stringi-search-charclass}).
 Defaults to `\code{\\P\{Wspace\}}.}

\item{side}{character [\code{stri_trim} only]; defaults to \code{'both'}}
}
\value{
All these functions return a character vector.
}
\description{
These functions may be used, e.g., to remove unnecessary
white-spaces from strings. Trimming ends at the first or
starts at the last \code{pattern} match.
}
\details{
Vectorized over \code{str} and \code{pattern}.

\code{stri_trim} is a convenience wrapper over \code{stri_trim_left}
and \code{stri_trim_right}.

Contrary to many other string processing libraries,
our trimming functions are universal. A character class,
given by \code{pattern}, may be adjusted to suit your needs
(yet, most often you stick to the default value).

For replacing pattern matches with
arbitrary replacement string, see \code{\link{stri_replace}}.

Trimming can also be used where you would normally rely on
regular expressions. For instance, you may get
\code{'23.5'} out of \code{'total of 23.5 bitcoins'}.

For trimming white-spaces, please note the difference
between Unicode binary property `\code{\\p\{Wspace\}}` (more universal)
and general character category `\code{\\p\{Z\}}`,
see \link{stringi-search-charclass}.
}
\examples{
stri_trim_left('               aaa')
stri_trim_right('r-project.org/', '\\\\p{P}')
stri_trim_both(' Total of 23.5 bitcoins. ', '\\\\p{N}')
stri_trim_both(' Total of 23.5 bitcoins. ', '\\\\p{L}')

}
\seealso{
Other search_replace: 
\code{\link{about_search}},
\code{\link{stri_replace_all}()}

Other search_charclass: 
\code{\link{about_search_charclass}},
\code{\link{about_search}}
}
\concept{search_charclass}
\concept{search_replace}
