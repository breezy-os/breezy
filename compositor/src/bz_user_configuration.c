
#include "breezy/bz_user_configuration.h"

#include <dirent.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>

#include "stb/stb_image.h"

#include "breezy/bz_breezy.h"
#include "breezy/bz_logger.h"


// =================================================================================================
//  File Variables / Declarations
// -------------------------------------------------------------------------------------------------

static bool bz_is_valid_image(const char *filename);

static char *bz_get_config_dir(void);
static char *bz_concat(const char *left, const char *right);

// =================================================================================================
//  Initialization / Teardown
// -------------------------------------------------------------------------------------------------

int bz_user_config_initialize(struct bz_user_config *conf)
{
	conf->config_dir = bz_get_config_dir();
	if (conf->config_dir == nullptr) {
		bz_error(BZ_LOG_USER_CONFIG, "Failed to look up user's home directory.");
		return 1;
	}

	return 0;
}

void bz_user_config_cleanup(struct bz_user_config *conf)
{
	if (conf->config_dir != nullptr) { free(conf->config_dir); }
}

void bz_load_random_desktop_background(struct bz_user_config *conf)
{
	// Open the user's backgrounds directory
	char *bg_dir = bz_concat(conf->config_dir, "/backgrounds/");
	DIR *dir = opendir(bg_dir);
	if (!dir) {
		bz_warn(BZ_LOG_USER_CONFIG, "Failed to list files in background directory.");
		goto close_dir;
	}

	// Count the number of valid background images in that directory
	struct dirent *entry;
	int file_count = 0;
	while ((entry = readdir(dir)) != nullptr) {
		if (bz_is_valid_image(entry->d_name)) { file_count++; }
	}
	rewinddir(dir);

	// Choose a random (valid) file to load
	int32_t file_index = bz_rand_int(0, file_count-1);
	while ((entry = readdir(dir)) != nullptr) {
		if (bz_is_valid_image(entry->d_name)) {
			if (file_index == 0) {
				char *bg_file = bz_concat(bg_dir, entry->d_name);

				// Load the file data
				int w, h, channels;
				unsigned char *data = stbi_load(bg_file, &w, &h, &channels, 4);
				if (!data) {
					bz_warn(BZ_LOG_USER_CONFIG, "Failed to load background image with stb_image.");
					return;
				}

				// Write the OpenGL texture
				glGenTextures(1, &conf->bg_texture);
				glBindTexture(GL_TEXTURE_2D, conf->bg_texture);
				glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
				glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);

				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

				stbi_image_free(data);
				free(bg_file);
				break;
			}
			file_index--;
		}
	}

	close_dir:
		closedir(dir);
		if (bg_dir != nullptr) free(bg_dir);
}

static bool bz_is_valid_image(const char *filename)
{
	if (!strcmp(filename, "."))  return false;
	if (!strcmp(filename, "..")) return false;

	const char *ext = strrchr(filename, '.');
	if (ext && strcasecmp(ext, ".png") == 0) return true;

	return false;
}

static char *bz_get_config_dir(void)
{
	// First, try the environment variable.
	const char *home = getenv("HOME");
	if (home == nullptr || *home == '\0') {
		// Fall back to user's registered home dir.
		struct passwd *pw = getpwuid(getuid());
		home = pw ? pw->pw_dir : nullptr;
	}

	// Sadly, no home to go to :'(
	if (home == nullptr) { return nullptr; }

	// Append breezy's config directory to it, returning the result.
	return bz_concat(home, "/.config/breezy");
}

static char *bz_concat(const char *left, const char *right)
{
	size_t full_len = strlen(left) + strlen(right) + 1;
	char *full = calloc(full_len, sizeof(char));
	if (full == nullptr) { return nullptr; }

	snprintf(full, full_len, "%s%s", left, right);
	return full;
}
