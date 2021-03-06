/* Copyright (c) 2005, 2015, Oracle and/or its affiliates. All rights reserved.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; version 2 of the License.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA */

// First include (the generated) my_config.h, to get correct platform defines.
#include "my_config.h"
#include <stdlib.h>
#include <ctype.h>
#include <mysql/plugin_ftparser.h>
#include <m_ctype.h>
#include "./jieba.h"

static const char* DICT_PATH = "/usr/share/dict/jieba.dict.utf8";
static const char* HMM_MODEL = "/usr/share/dict/hmm_model.utf8";
static const char* USER_DICT_PATH = "/usr/share/dict/user.dict.utf8";

static void* jieba_hanlde = NULL;

/*
  sqljieba interface functions:

  Plugin declaration functions:
  - sqljieba_plugin_init()
  - sqljieba_plugin_deinit()

  Parser descriptor functions:
  - sqljieba_parse()
  - sqljieba_init()
  - sqljieba_deinit()
*/


/*
  Initialize the plugin at server start or plugin installation.
  NOTICE: when the DICT_PATH, HMM_MODEL, USER_DICT_PATH not found, NewJieba would log the error and exit without return anything .

  SYNOPSIS
    sqljieba_plugin_init()

  DESCRIPTION
    Does nothing.

  RETURN VALUE
    0                    success
    1                    failure (cannot happen)
*/

static int sqljieba_plugin_init(void *arg __attribute__((unused)))
{
  jieba_hanlde = NewJieba(DICT_PATH, HMM_MODEL, USER_DICT_PATH);
  return(0);
}

static int sqljieba_plugin_deinit(void *arg __attribute__((unused)))
{
  FreeJieba(jieba_hanlde);
  return(0);
}


/*
  Initialize the parser on the first use in the query

  SYNOPSIS
    sqljieba_init()

  DESCRIPTION
    Does nothing.

  RETURN VALUE
    0                    success
    1                    failure (cannot happen)
*/

static int sqljieba_init(MYSQL_FTPARSER_PARAM *param
                              __attribute__((unused)))
{
  return(0);
}


/*
  Terminate the parser at the end of the query

  SYNOPSIS
    sqljieba_deinit()

  DESCRIPTION
    Does nothing.

  RETURN VALUE
    0                    success
    1                    failure (cannot happen)
*/

static int sqljieba_deinit(MYSQL_FTPARSER_PARAM *param
                                __attribute__((unused)))
{
  return(0);
}


/*
  Pass a word back to the server.

  SYNOPSIS
    add_word()
      param              parsing context of the plugin
      word               a word
      len                word length

  DESCRIPTION
    Fill in boolean metadata for the word (if parsing in boolean mode)
    and pass the word to the server.  The server adds the word to
    a full-text index when parsing for indexing, or adds the word to
    the list of search terms when parsing a search string.
*/

static void add_word(MYSQL_FTPARSER_PARAM *param, char *word, size_t len)
{
  MYSQL_FTPARSER_BOOLEAN_INFO bool_info=
    { FT_TOKEN_WORD, 0, 0, 0, 0, (word - param->doc), ' ', 0 };

  param->mysql_add_word(param, word, len, &bool_info);
}

/*
  Parse a document or a search query.

  SYNOPSIS
    sqljieba_parse()
      param              parsing context

  DESCRIPTION
    This is the main plugin function which is called to parse
    a document or a search query. The call mode is set in
    param->mode.  This function simply splits the text into words
    and passes every word to the MySQL full-text indexing engine.

  RETURN VALUE
    0                    success
    1                    failure (cannot happen)
*/

static int sqljieba_parse(MYSQL_FTPARSER_PARAM *param)
{
  assert(jieba_hanlde);
  CJiebaWord* words = Cut(jieba_hanlde, param->doc, param->length);
  CJiebaWord* x;
  for (x = words; x && x->word; x++) {
    add_word(param, (char*)x->word, x->len);
  }
  FreeWords(words);
  return(0);
}


/*
  Plugin type-specific descriptor
*/

static struct st_mysql_ftparser sqljieba_descriptor=
{
  MYSQL_FTPARSER_INTERFACE_VERSION, /* interface version      */
  sqljieba_parse,              /* parsing function       */
  sqljieba_init,               /* parser init function   */
  sqljieba_deinit              /* parser deinit function */
};

/*
  Plugin status variables for SHOW STATUS
*/

static struct st_mysql_show_var simple_status[]=
{
  {"static",     (char *)"just a static text",     SHOW_CHAR, SHOW_SCOPE_GLOBAL},
  {0,0,0, SHOW_SCOPE_GLOBAL}
};

/*
  Plugin system variables.
*/

static long     sysvar_one_value;
static char     *sysvar_two_value;

static MYSQL_SYSVAR_LONG(simple_sysvar_one, sysvar_one_value,
  PLUGIN_VAR_RQCMDARG,
  "Simple fulltext parser example system variable number one. Give a number.",
  NULL, NULL, 77L, 7L, 777L, 0);

static MYSQL_SYSVAR_STR(simple_sysvar_two, sysvar_two_value,
  PLUGIN_VAR_RQCMDARG | PLUGIN_VAR_MEMALLOC,
  "Simple fulltext parser example system variable number two. Give a string.",
  NULL, NULL, "simple sysvar two default");

static MYSQL_THDVAR_LONG(simple_thdvar_one,
  PLUGIN_VAR_RQCMDARG,
  "Simple fulltext parser example thread variable number one. Give a number.",
  NULL, NULL, 88L, 8L, 888L, 0);

static MYSQL_THDVAR_STR(simple_thdvar_two,
  PLUGIN_VAR_RQCMDARG | PLUGIN_VAR_MEMALLOC,
  "Simple fulltext parser example thread variable number two. Give a string.",
  NULL, NULL, "simple thdvar two default");

static struct st_mysql_sys_var* simple_system_variables[]= {
  MYSQL_SYSVAR(simple_sysvar_one),
  MYSQL_SYSVAR(simple_sysvar_two),
  MYSQL_SYSVAR(simple_thdvar_one),
  MYSQL_SYSVAR(simple_thdvar_two),
  NULL
};

/*
  Plugin library descriptor
*/

mysql_declare_plugin(ftexample)
{
  MYSQL_FTPARSER_PLUGIN,      /* type                            */
  &sqljieba_descriptor,  /* descriptor                      */
  "sqljieba",            /* name                            */
  "github.com/yanyiwu",              /* author                          */
  "Jieba Full-Text Parser",  /* description                     */
  PLUGIN_LICENSE_GPL,
  sqljieba_plugin_init,  /* init function (when loaded)     */
  sqljieba_plugin_deinit,/* deinit function (when unloaded) */
  0x0001,                     /* version                         */
  simple_status,              /* status variables                */
  simple_system_variables,    /* system variables                */
  NULL,
  0,
}
mysql_declare_plugin_end;

