/* A Bison parser, made by GNU Bison 2.7.  */

/* Bison interface for Yacc-like parsers in C
   
      Copyright (C) 1984, 1989-1990, 2000-2012 Free Software Foundation, Inc.
   
   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   
   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.
   
   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */

#ifndef YY_I386_I_PARSE_H_INCLUDED
# define YY_I386_I_PARSE_H_INCLUDED
/* Enabling traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif
#if YYDEBUG
extern int i386_debug;
#endif

/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum yytokentype {
     kMASK = 258,
     kPREFIX = 259,
     kSUFFIX = 260,
     kSYNONYM = 261,
     kID = 262,
     kNUMBER = 263,
     kPERCPERC = 264,
     kBITFIELD = 265,
     kCHAR = 266,
     kSPACE = 267
   };
#endif
/* Tokens.  */
#define kMASK 258
#define kPREFIX 259
#define kSUFFIX 260
#define kSYNONYM 261
#define kID 262
#define kNUMBER 263
#define kPERCPERC 264
#define kBITFIELD 265
#define kCHAR 266
#define kSPACE 267



#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE
{
/* Line 2058 of yacc.c  */
#line 217 "/home/mark/src/elfutils/libcpu/i386_parse.y"

  unsigned long int num;
  char *str;
  char ch;
  struct known_bitfield *field;
  struct bitvalue *bit;
  struct argname *name;
  struct argument *arg;


/* Line 2058 of yacc.c  */
#line 92 "i386_parse.h"
} YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
#endif

extern YYSTYPE i386_lval;

#ifdef YYPARSE_PARAM
#if defined __STDC__ || defined __cplusplus
int i386_parse (void *YYPARSE_PARAM);
#else
int i386_parse ();
#endif
#else /* ! YYPARSE_PARAM */
#if defined __STDC__ || defined __cplusplus
int i386_parse (void);
#else
int i386_parse ();
#endif
#endif /* ! YYPARSE_PARAM */

#endif /* !YY_I386_I_PARSE_H_INCLUDED  */
