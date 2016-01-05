/*
  Copyright (C) 2016 by Syohei YOSHIDA

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <curl/curl.h>
#include <string.h>
#include <stdlib.h>

#include <emacs-module.h>

int plugin_is_GPL_compatible;

struct memstruct {
	char *data;
	size_t size;
};

static size_t
write_callback(char *ptr, size_t size, size_t nmemb, void *userp)
{
	size_t realsize = size * nmemb;
	struct memstruct *s = (struct memstruct*)userp;

	s->data = realloc(s->data, s->size + realsize + 1);
	if (s->data == NULL) {
		return 0;
	}

	memcpy(&(s->data[s->size]), ptr, realsize);
	s->size += realsize;
	s->data[s->size] = '\0';

	return realsize;
}

static emacs_value
Fcurl_get(emacs_env *env, ptrdiff_t nargs, emacs_value args[], void *data)
{
	ptrdiff_t len = 0;
	emacs_value url = args[0];
	env->copy_string_contents(env, url, NULL, &len);

	char *buf = malloc(len);
	if (buf == NULL)
		return env->intern(env, "nil");

	env->copy_string_contents(env, url, buf, &len);

	CURL *curl = curl_easy_init();
	struct memstruct chunk;
	chunk.data = NULL;
	chunk.size = 0;

	curl_easy_setopt(curl, CURLOPT_URL, buf);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);

	CURLcode res = curl_easy_perform(curl);
	free(buf);

	if (res != CURLE_OK) {
		fprintf(stderr, "curl_easy_perform() failed: %s\n",
			curl_easy_strerror(res));
		free(chunk.data);
		curl_easy_cleanup(curl);
		return env->intern(env, "nil");
	}

	emacs_value ret = env->make_string(env, chunk.data, chunk.size-1);
	free(chunk.data);
	curl_easy_cleanup(curl);
	return ret;
}

static void
bind_function(emacs_env *env, const char *name, emacs_value Sfun)
{
	emacs_value Qfset = env->intern(env, "fset");
	emacs_value Qsym = env->intern(env, name);
	emacs_value args[] = { Qsym, Sfun };

	env->funcall(env, Qfset, 2, args);
}

static void
provide(emacs_env *env, const char *feature)
{
	emacs_value Qfeat = env->intern(env, feature);
	emacs_value Qprovide = env->intern (env, "provide");
	emacs_value args[] = { Qfeat };

	env->funcall(env, Qprovide, 1, args);
}

int
emacs_module_init(struct emacs_runtime *ert)
{
	emacs_env *env = ert->get_environment(ert);

#define DEFUN(lsym, csym, amin, amax, doc, data) \
	bind_function (env, lsym, env->make_function(env, amin, amax, csym, doc, data))

	DEFUN("curl-get", Fcurl_get, 1, 1, "GET content", NULL);
#undef DEFUN

	provide(env, "curl-core");
	return 0;
}

/*
  Local Variables:
  c-basic-offset: 8
  indent-tabs-mode: t
  End:
*/
