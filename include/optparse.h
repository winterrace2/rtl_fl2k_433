/**
 * Option parsing functions to complement getopt.
 *
 * Copyright (C) 2017 Christian Zuckschwerdt
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef INCLUDE_OPTPARSE_H_
#define INCLUDE_OPTPARSE_H_

#include <stdint.h>

// makes strcasecmp() and strncasecmp() available when including optparse.h
#ifdef _MSC_VER
    #include <string.h>
    #define strcasecmp(s1,s2)     _stricmp(s1,s2)
    #define strncasecmp(s1,s2,n)  _strnicmp(s1,s2,n)
#else
    #include <strings.h>
#endif

/// Convert string to int with fallback default.
int atoiv(char *arg, int def);

/// Convert string to bool with fallback default.
/// Parses "true", "yes", "on", "enable" (not case-sensitive) to 1, atoi() otherwise.
int atobv(char *arg, int def);

/// Get the next colon or comma separated arg, NULL otherwise.
/// Returns string including comma if a comma is found first,
/// otherwise string after colon if found, NULL otherwise.
char *arg_param(char *arg);

/// Parse param string to host and port.
/// E.g. ":514", "localhost", "[::1]", "127.0.0.1:514", "[::1]:514",
/// also "//localhost", "//localhost:514", "//:514".
/// Host or port are terminated at a comma, if found.
/// @return the remaining options (might be "") or NULL on parsing error
char *hostport_param(char *param, char **host, char **port);

/// Convert a string to an unsigned integer, uses strtod() and accepts
/// metric suffixes of 'k', 'M', and 'G' (also 'K', 'm', and 'g').
///
/// Parse errors will Gui_fprintf(stderr, ...) and exit(1).
///
/// @param str: character string to parse
/// @param error_hint: prepended to error output
/// @return parsed number value
uint32_t atouint32_metric(const char *str, const char *error_hint);

/// Convert a string to an integer, uses strtod() and accepts
/// time suffixes of 's', 'm', and 'h' (also 'S', 'M', and 'H').
///
/// Parse errors will Gui_fprintf(stderr, ...) and exit(1).
///
/// @param str: character string to parse
/// @param error_hint: prepended to error output
/// @return parsed number value
int atoi_time(const char *str, const char *error_hint);

/// Trim left and right whitespace in string.
///
/// @param[in,out] str
/// @return the trimmed value of str
char *trim_ws(char *str);

/// Remove all whitespace from string.
///
/// @param[in,out] str
/// @return the stripped value of str
char *remove_ws(char *str);

#endif /* INCLUDE_OPTPARSE_H_ */
