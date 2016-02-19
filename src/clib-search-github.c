#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "case/case.h"
#include "commander/commander.h"
#include "tempdir/tempdir.h"
#include "fs/fs.h"
#include "http-get/http-get.h"
#include "asprintf/asprintf.h"
#include "wiki-registry/wiki-registry.h"
#include "clib-package/clib-package.h"
#include "strdup/strdup.h"
#include "logger/logger.h"
#include "debug/debug.h"
#include "parson/parson.h"
#include "version.h"

const char        *clib_search_cache      = "clib-search.cache";
const int         clib_search_cache_time  = 1000 * 60 * 60 * 5;
const char        *programstr             = "clib-search-github";
static debug_t    debugger;
static int        opt_cache;
static char       *opt_url                = "https://github.com/tencherry10/clib/wiki/Packages";

static void setup_args(command_t *self, int argc, char **argv) {
  void setopt_nocache(command_t *self) {
    opt_cache = 0;
    debug(&debugger, "set enable cache: %d", opt_cache);
  }
  void setopt_url(command_t *self) {
    opt_url = (char *) self->arg;
    debug(&debugger, "set mkopts: %s", opt_url);
  }
  self->usage = "[options] [query ...]";
  command_option(self, "-c", "--skip-cache",  "skip the search cache", setopt_nocache);  
  command_option(self, "-u", "--url [url]",   "set the url to fetch from", setopt_nocache);  
  command_parse(self, argc, argv);
  for (int i = 0; i < self->argc; i++) case_lower(self->argv[i]);  
}

static char * clib_search_file(void) {
  char *file = NULL;
  char *temp = NULL;

  temp = gettempdir();
  if (NULL == temp) {
    logger_error("error", "gettempdir() out of memory");
    return NULL;
  }

  debug(&debugger, "tempdir: %s", temp);
  int rc = asprintf(&file, "%s/%s", temp, clib_search_cache);
  if (-1 == rc) {
    logger_error("error", "asprintf() out of memory");
    free(temp);
    return NULL;
  }

  free(temp);
  debug(&debugger, "search file: %s", file);
  return file;
}

static char * wiki_html_cache() {
  char *cache_file = clib_search_file();
  if (NULL == cache_file) return NULL;

  if (0 == opt_cache) {
    debug(&debugger, "skipping cache file (%s)", cache_file);
    goto set_cache;
  }

  fs_stats *stats = fs_stat(cache_file);
  if (NULL == stats) goto set_cache;

  long now = (long) time(NULL);
  long modified = stats->st_mtime;
  long delta = now - modified;

  debug(&debugger, "cache delta %d (%d - %d)", delta, now, modified);
  free(stats);

  if (delta < clib_search_cache_time) {
    char *data = fs_read(cache_file);
    free(cache_file);
    return data;
  }

set_cache:;
  debug(&debugger, "setting cache (%s) from %s", cache_file, opt_url);
  http_get_response_t *res = http_get(opt_url);
  if (!res->ok) return NULL;

  char *html = strdup(res->data);
  if (NULL == html) return NULL;
  http_get_free(res);

  if (NULL == html) return html;
  fs_write(cache_file, html);
  debug(&debugger, "wrote cache (%s)", cache_file);
  free(cache_file);
  return html;
}

int main(int argc, char *argv[]) {
  command_t  program;  
  JSON_Value  *rootval    = json_value_init_object();
  JSON_Object *rootobj    = json_value_get_object(rootval);
  JSON_Value  *pkgarrval  = json_value_init_array();
  JSON_Array  *pkgarr     = json_value_get_array(pkgarrval);
  char *serialized_string = NULL;
  opt_cache               = 1;
  debug_init(&debugger, programstr);
  command_init(&program, programstr, CLIB_VERSION);
  setup_args(&program, argc, argv);

  char *html = wiki_html_cache();
  if (NULL == html) {
    command_free(&program);
    logger_error("error", "failed to fetch wiki HTML");
    return 1;
  }
  list_t *pkgs = wiki_registry_parse(html);
  free(html);
  
  debug(&debugger, "found %zu packages", pkgs->len);
  
  list_node_t *node;
  list_iterator_t *it = list_iterator_new(pkgs, LIST_HEAD);
  while ((node = list_iterator_next(it))) {
    JSON_Value  *pkgobjval  = json_value_init_object();
    JSON_Object *pkgobj     = json_value_get_object(pkgobjval);    
    wiki_package_t *pkg = (wiki_package_t *) node->val;
    json_object_set_string(pkgobj, "repo", pkg->repo);
    json_object_set_string(pkgobj, "url", pkg->href);
    json_object_set_string(pkgobj, "desc", pkg->description);
    wiki_package_free(pkg);
    json_array_append_value(pkgarr, pkgobjval);
  }
  list_iterator_destroy(it);
  list_destroy(pkgs);
  
  json_object_set_value(rootobj, "pkglist", pkgarrval);
  json_object_set_string(rootobj, "program", programstr);
  
  serialized_string = json_serialize_to_string(rootval);
  printf("%s\n", serialized_string);
  
  json_free_serialized_string(serialized_string);
  json_value_free(rootval);
  json_value_free(pkgarrval);
  command_free(&program);
  return 0;
}
