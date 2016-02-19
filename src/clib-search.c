#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "case/case.h"
#include "commander/commander.h"
#include "tempdir/tempdir.h"
#include "fs/fs.h"
#include "asprintf/asprintf.h"
#include "clib-package/clib-package.h"
#include "console-colors/console-colors.h"
#include "strdup/strdup.h"
#include "logger/logger.h"
#include "debug/debug.h"
#include "kvec/kvec.h"
#include "sds/sds.h"
#include "trim/trim.h"
#include "parson/parson.h"
#include "version.h"

typedef kvec_t(char*) string_vec_t;

static debug_t  debugger;
static int      opt_color = 1;
static int      opt_cache = 1;

static void setup_args(command_t *self, int argc, char **argv) {
  void setopt_nocache(command_t *self) {
    opt_cache = 0;
    debug(&debugger, "set enable cache: %d", opt_cache);
  }
  void setopt_nocolor(command_t *self) {
    opt_color = 0;
    debug(&debugger, "set opt_color = %d", opt_color);
  }
  self->usage = "[options] [query ...]";
  command_option(self, "-c", "--skip-cache",  "skip the search cache", setopt_nocache);  
  command_option(self, "-n", "--no-color",    "don't colorize output", setopt_nocolor);
  command_parse(self, argc, argv);
  for (int i = 0; i < self->argc; i++) case_lower(self->argv[i]);  
  //~ // set color theme
  //~ cc_color_t fg_color_highlight = opt_color ? CC_FG_DARK_CYAN : CC_FG_NONE;
  //~ cc_color_t fg_color_text = opt_color ? CC_FG_DARK_GRAY : CC_FG_NONE;  
}

#if 0
static int matches(int count, char *args[], wiki_package_t *pkg) {
  // Display all packages if there's no query
  if (0 == count) return 1;

  char *name = NULL;
  char *description = NULL;

  name = clib_package_parse_name(pkg->repo);
  if (NULL == name) goto fail;
  case_lower(name);
  for (int i = 0; i < count; i++) {
    if (strstr(name, args[i])) {
      free(name);
      return 1;
    }
  }

  description = strdup(pkg->description);
  if (NULL == description) goto fail;
  case_lower(description);
  for (int i = 0; i < count; i++) {
    if (strstr(description, args[i])) {
      free(description);
      free(name);
      return 1;
    }
  }

fail:
  free(name);
  free(description);
  return 0;
}
#endif

string_vec_t exec_that_starts_with(const char *prefix) {
  FILE          *cmdfp;
  char          cmdlinebuf[1024] = {0};
  const char    *cmdfmt = "IFS=:; find $PATH -maxdepth 1 -executable -type f -printf '%%f\\n' | sort | grep -e \"^%s\"";
  char          *cmdstr = NULL;
  char          *cmd;
  string_vec_t  ret     = {0,0,0};
  
  if(-1 == asprintf(&cmdstr, cmdfmt, prefix)) 
    return ret;
  
  if( (cmdfp = popen(cmdstr, "r")) == NULL) 
    goto cleanup;
  
  while(fgets(cmdlinebuf, sizeof(cmdlinebuf)-1, cmdfp) != NULL) {
    if((cmd = strdup(trim(cmdlinebuf))) == NULL)
      goto cleanup_both;
    debug(&debugger, "cmd: %s", cmd);
    kv_push(char*, ret, cmd);
  }

  cleanup_both:  
  pclose(cmdfp);
cleanup:
  free(cmdstr);
  return ret;
}

sds run_cmd(sds s, const char * cmd) {
  FILE          *cmdfp;
  sdsclear(s);
  
  if( (cmdfp = popen(cmd, "r")) == NULL) 
    return s;
  
  while(!feof(cmdfp)) {
    s = sdsMakeRoomFor(s, 4096);
    size_t oldlen = sdslen(s);
    size_t numread = fread(s + oldlen, 1, 4096, cmdfp);
    if(numread < 0) {
      sdsclear(s);
      return s;
    }
    sdsIncrLen(s, numread);
  }
  return s;
}

int main(int argc, char *argv[]) {
  command_t program;
  debug_init(&debugger, "clib-search");
  command_init(&program, "clib-search", CLIB_VERSION);
  setup_args(&program, argc, argv);
  string_vec_t cmd_list = exec_that_starts_with("clib-search-");
  
  sds s = sdsempty();
  for(int i = 0 ; i < kv_size(cmd_list) ; i++) {
    char *  cmd = kv_A(cmd_list, i);
    char    cmdbuf[strlen(cmd) + 16];
    sprintf(cmdbuf, "%s %s", cmd, opt_cache ? "" : "-c");
    s = run_cmd(s, cmdbuf);
    JSON_Value * parsed = json_parse_string(s);
    printf("%s\n", s);
    json_value_free(parsed);
  }
  
  
  for(int i = 0 ; i < kv_size(cmd_list) ; i++) {
    free(kv_A(cmd_list, i));
  }
  
  kv_destroy(cmd_list);
  command_free(&program);
  return 0;
}
