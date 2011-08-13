#include <liblrzip_private.h>

#ifdef HAVE_SYS_TIME_H
# include <sys/time.h>
#endif
#ifdef HAVE_SYS_RESOURCE_H
# include <sys/resource.h>
#endif
#ifdef HAVE_SYS_TYPES_H
# include <sys/types.h>
#endif
#ifdef HAVE_SYS_STAT_H
# include <sys/stat.h>
#endif

/* needed for CRC routines */
#include "lzma/C/7zCrc.h"
#include "util.h"
#include "lrzip.h"
#include "rzip.h"

static void liblrzip_index_update(size_t x, size_t *idx, void **queue)
{
	for (; x < *idx; x++)
		queue[x] = queue[x] + 1;
	(*idx)--;
}

static bool liblrzip_setup_flags(Lrzip *lr)
{
	if (!lr) return false;
#define MODE_CHECK(X) \
	case LRZIP_MODE_COMPRESS_##X: \
	lr->control->flags ^= FLAG_NOT_LZMA; \
	lr->control->flags |= FLAG_##X##_COMPRESS; \
	break


	switch (lr->mode) {
	case LRZIP_MODE_DECOMPRESS:
		lr->control->flags |= FLAG_DECOMPRESS;
		break;
	case LRZIP_MODE_TEST:
		lr->control->flags |= FLAG_TEST_ONLY;
		break;
	case LRZIP_MODE_INFO:
		lr->control->flags |= FLAG_INFO;
		break;
	case LRZIP_MODE_COMPRESS_NONE:
		lr->control->flags ^= FLAG_NOT_LZMA;
		lr->control->flags |= FLAG_NO_COMPRESS;
		break;
	case LRZIP_MODE_COMPRESS_LZMA:
		lr->control->flags ^= FLAG_NOT_LZMA;
		break;
	MODE_CHECK(LZO);
	MODE_CHECK(BZIP2);
	MODE_CHECK(ZLIB);
	MODE_CHECK(ZPAQ);
#undef MODE_CHECK
	default:
		return false;
	}
	setup_overhead(lr->control);
	if (lr->flags & LRZIP_FLAG_VERIFY) {
		lr->control->flags |= FLAG_CHECK;
		lr->control->flags |= FLAG_HASH;
	}
	if (lr->flags & LRZIP_FLAG_REMOVE_DESTINATION)
		lr->control->flags |= FLAG_FORCE_REPLACE;
	if (lr->flags & LRZIP_FLAG_REMOVE_SOURCE)
		lr->control->flags &= ~FLAG_KEEP_FILES;
	if (lr->flags & LRZIP_FLAG_KEEP_BROKEN)
		lr->control->flags |= FLAG_KEEP_BROKEN;
	if (lr->flags & LRZIP_FLAG_DISABLE_LZO_CHECK)
		lr->control->flags &= ~FLAG_THRESHOLD;
	if (lr->flags & LRZIP_FLAG_UNLIMITED_RAM)
		lr->control->flags |= FLAG_UNLIMITED;
	if (lr->flags & LRZIP_FLAG_ENCRYPT)
		lr->control->flags |= FLAG_ENCRYPT;
	if (lr->control->log_level > 0) {
		lr->control->flags |= FLAG_SHOW_PROGRESS;
		if (lr->control->log_level > 1) {
			lr->control->flags |= FLAG_VERBOSITY;
			if (lr->control->log_level > 2)
				lr->control->flags |= FLAG_VERBOSITY_MAX;
		}
	} else lr->control->flags ^= (FLAG_VERBOSE | FLAG_SHOW_PROGRESS);
	return true;
}


bool lrzip_init(void)
{
	/* generate crc table */
	CrcGenerateTable();
	return true;
}

void lrzip_config_env(Lrzip *lr)
{
	const char *eptr;
	/* Get Preloaded Defaults from lrzip.conf
	 * Look in ., $HOME/.lrzip/, /etc/lrzip.
	 * If LRZIP=NOCONFIG is set, then ignore config
	 */
	eptr = getenv("LRZIP");
	if (!eptr)
		read_config(lr->control);
	else if (!strstr(eptr,"NOCONFIG"))
		read_config(lr->control);
}

void lrzip_free(Lrzip *lr)
{
	size_t x;

	if ((!lr) || (!lr->infilename_buckets)) return;
	rzip_control_free(lr->control);
	for (x = 0; x < lr->infilename_idx; x++)
		free(lr->infilenames[x]);
	free(lr->infilenames);
	free(lr->infiles);
	free(lr);
}

Lrzip *lrzip_new(Lrzip_Mode mode)
{
	Lrzip *lr;

	lr = calloc(1, sizeof(Lrzip));
	if (!lr) return NULL;
	lr->control = calloc(1, sizeof(rzip_control));
	if (!lr->control) goto error;
	if (!initialize_control(lr->control)) goto error;
	lr->mode = mode;
	lr->control->library_mode = 1;
	return lr;
error:
	lrzip_free(lr);
	return NULL;
}

Lrzip_Mode lrzip_mode_get(Lrzip *lr)
{
	if (!lr) return LRZIP_MODE_NONE;
	return lr->mode;
}

bool lrzip_mode_set(Lrzip *lr, Lrzip_Mode mode)
{
	if ((!lr) || (mode > LRZIP_MODE_COMPRESS_ZPAQ)) return false;
	lr->mode = mode;
	return true;
}

bool lrzip_compression_level_set(Lrzip *lr, unsigned int level)
{
	if ((!lr) || (!level) || (level > 9)) return false;
	lr->control->compression_level = level;
	return true;
}

unsigned int lrzip_compression_level_get(Lrzip *lr)
{
   if (!lr) return 0;
   return lr->control->compression_level;
}

void lrzip_flags_set(Lrzip *lr, unsigned int flags)
{
	if (!lr) return;
	lr->flags = flags;
}

unsigned int lrzip_flags_get(Lrzip *lr)
{
	if (!lr) return 0;
	return lr->flags;
}

void lrzip_nice_set(Lrzip *lr, int nice)
{
	if ((!lr) || (nice < -19) || (nice > 20)) return;
	lr->control->nice_val = nice;
}

int lrzip_nice_get(Lrzip *lr)
{
	if (!lr) return 0;
	return lr->control->nice_val;
}

void lrzip_threads_set(Lrzip *lr, unsigned int threads)
{
	if ((!lr) || (!threads)) return;
	lr->control->threads = threads;
}

unsigned int lrzip_threads_get(Lrzip *lr)
{
	if (!lr) return 0;
	return lr->control->threads;
}

void lrzip_compression_window_max_set(Lrzip *lr, int64_t size)
{
	if (!lr) return;
	lr->control->window = size;
}

int64_t lrzip_compression_window_max_get(Lrzip *lr)
{
	if (!lr) return -1;
	return lr->control->window;
}

unsigned int lrzip_files_count(Lrzip *lr)
{
	if (!lr) return 0;
	return lr->infile_idx;
}

unsigned int lrzip_filenames_count(Lrzip *lr)
{
	if (!lr) return 0;
	return lr->infilename_idx;
}

FILE **lrzip_files_get(Lrzip *lr)
{
	if (!lr) return NULL;
	return lr->infiles;
}

char **lrzip_filenames_get(Lrzip *lr)
{
	if (!lr) return NULL;
	return lr->infilenames;
}

bool lrzip_file_add(Lrzip *lr, FILE *file)
{
	if ((!lr) || (!file)) return false;
	if (lr->infilenames) return false;
	if (!lr->infile_buckets) {
		/* no files added */
		lr->infiles = calloc(INFILE_BUCKET_SIZE + 1, sizeof(void*));
		lr->infile_buckets++;
	} else if (lr->infile_idx == INFILE_BUCKET_SIZE * lr->infile_buckets + 1) {
		/* all buckets full, create new bucket */
		FILE **tmp;

		tmp = realloc(lr->infiles, (++lr->infile_buckets * INFILE_BUCKET_SIZE + 1) * sizeof(void*));
		if (!tmp) return false;
		lr->infiles = tmp;
	}

	lr->infiles[lr->infile_idx++] = file;
	return true;
}

bool lrzip_file_del(Lrzip *lr, FILE *file)
{
	size_t x;

	if ((!lr) || (!file)) return false;
	if (!lr->infile_buckets) return true;

	for (x = 0; x <= lr->infile_idx + 1; x++) {
		if (!lr->infiles[x]) return true; /* not found */
		if (lr->infiles[x] != file) continue; /* not a match */
		break;
	}
	/* update index */
	liblrzip_index_update(x, &lr->infile_idx, (void**)lr->infiles);
	return true;
}

void lrzip_files_clear(Lrzip *lr)
{
	if ((!lr) || (!lr->infile_buckets)) return;
	free(lr->infiles);
	lr->infiles = NULL;
}

bool lrzip_filename_add(Lrzip *lr, const char *file)
{
	struct stat st;

	if ((!lr) || (!file) || (!file[0]) || (!strcmp(file, "-"))) return false;
	if (lr->infiles) return false;
	if (stat(file, &st)) return false;
	if (S_ISDIR(st.st_mode)) return false;

	if (!lr->infilename_buckets) {
		/* no files added */
		lr->infilenames = calloc(INFILE_BUCKET_SIZE + 1, sizeof(void*));
		lr->infilename_buckets++;
	} else if (lr->infilename_idx == INFILE_BUCKET_SIZE * lr->infilename_buckets + 1) {
		/* all buckets full, create new bucket */
		char **tmp;

		tmp = realloc(lr->infilenames, (++lr->infilename_buckets * INFILE_BUCKET_SIZE + 1) * sizeof(void*));
		if (!tmp) return false;
		lr->infilenames = tmp;
	}

	lr->infilenames[lr->infilename_idx++] = strdup(file);
	return true;
}

bool lrzip_filename_del(Lrzip *lr, const char *file)
{
	size_t x;

	if ((!lr) || (!file) || (!file[0])) return false;
	if (!lr->infilename_buckets) return true;

	for (x = 0; x <= lr->infilename_idx + 1; x++) {
		if (!lr->infilenames[x]) return true; /* not found */
		if (strcmp(lr->infilenames[x], file)) continue; /* not a match */
		free(lr->infilenames[x]);
		break;
	}
	/* update index */
	liblrzip_index_update(x, &lr->infilename_idx, (void**)lr->infilenames);
	return true;
}

void lrzip_filenames_clear(Lrzip *lr)
{
	size_t x;
	if ((!lr) || (!lr->infilename_buckets)) return;
	for (x = 0; x < lr->infilename_idx; x++)
		free(lr->infilenames[x]);
	free(lr->infilenames);
	lr->infilenames = NULL;
}

void lrzip_suffix_set(Lrzip *lr, const char *suffix)
{
	if ((!lr) || (!suffix) || (!suffix[0])) return;
	free(lr->control->suffix);
	lr->control->suffix = strdup(suffix);
}

const char *lrzip_suffix_get(Lrzip *lr)
{
	if (!lr) return NULL;
	return lr->control->suffix;
}

void lrzip_outdir_set(Lrzip *lr, const char *dir)
{
	const char *slash;
	char *buf;
	size_t len;
	if ((!lr) || (!dir) || (!dir[0])) return;
	free(lr->control->outdir);
	slash = strrchr(dir, '/');
	if (slash && (slash[1] == 0)) {
		lr->control->outdir = strdup(dir);
		return;
	}
	len = strlen(dir);
	buf = malloc(len + 2);
	if (!buf) return;
	memcpy(buf, dir, len);
	buf[len] = '/';
	buf[len + 1] = 0;
	lr->control->outdir = buf;
}

const char *lrzip_outdir_get(Lrzip *lr)
{
	if (!lr) return NULL;
	return lr->control->outdir;
}

void lrzip_outfile_set(Lrzip *lr, FILE *file)
{
	if ((!lr) || (file && (file == stderr))) return;
	if (lr->control->outname) return;
	lr->control->outFILE = file;
}

FILE *lrzip_outfile_get(Lrzip *lr)
{
	if (!lr) return NULL;
	return lr->control->outFILE;
}

void lrzip_outfilename_set(Lrzip *lr, const char *file)
{
	if ((!lr) || (file && (!file[0]))) return;
	if (lr->control->outFILE) return;
	if (lr->control->outname && file && (!strcmp(lr->control->outname, file))) return;
	free(lr->control->outname);
	lr->control->outname = file ? strdup(file) : NULL;
}

const char *lrzip_outfilename_get(Lrzip *lr)
{
	if (!lr) return NULL;
	return lr->control->outname;
}

const unsigned char *lrzip_md5digest_get(Lrzip *lr)
{
	if (!lr) return NULL;
	return lr->control->md5_resblock;
}

bool lrzip_run(Lrzip *lr)
{
	struct timeval start_time, end_time;
	rzip_control *control;
	double seconds,total_time; // for timers
	int hours,minutes;

	if (!liblrzip_setup_flags(lr)) return false;
	control = lr->control;

	if ((!lr->infile_idx) && (!lr->infilename_idx)) return false;
	if (lr->control->outFILE) {
		if (lr->control->outFILE == lr->control->msgout)
			lr->control->msgout = stderr;
		lr->control->flags |= FLAG_STDOUT;
		register_outputfile(lr->control, lr->control->msgout);
	}

	if (lr->infilenames)
		lr->control->infile = lr->infilenames[0];
	else {
		lr->control->inFILE = lr->infiles[0];
		control->flags |= FLAG_STDIN;
	}

	if ((!STDOUT) && (!lr->control->msgout)) lr->control->msgout = stdout;
	register_outputfile(lr->control, lr->control->msgout);

	setup_ram(lr->control);

	gettimeofday(&start_time, NULL);

	if (ENCRYPT && (!lr->control->pass_cb)) {
		print_err("No password callback set!\n");
		return false;
	}

	if (DECOMPRESS || TEST_ONLY) {
		if (!decompress_file(lr->control)) return false;
	} else if (INFO) {
		if (!get_fileinfo(lr->control)) return false;
	} else if (!compress_file(lr->control)) return false;

	/* compute total time */
	gettimeofday(&end_time, NULL);
	total_time = (end_time.tv_sec + (double)end_time.tv_usec / 1000000) -
		      (start_time.tv_sec + (double)start_time.tv_usec / 1000000);
	hours = (int)total_time / 3600;
	minutes = (int)(total_time / 60) % 60;
	seconds = total_time - hours * 3600 - minutes * 60;
	if (!INFO)
		print_progress("Total time: %02d:%02d:%05.2f\n", hours, minutes, seconds);

	return true;
}

void lrzip_log_level_set(Lrzip *lr, int level)
{
	if (!lr) return;
	lr->control->log_level = level;
}

int lrzip_log_level_get(Lrzip *lr)
{
	if (!lr) return 0;
	return lr->control->log_level;
}

void lrzip_log_cb_set(Lrzip *lr, Lrzip_Log_Cb cb, void *log_data)
{
	if (!lr) return;
	lr->control->log_cb = (void*)cb;
	lr->control->log_data = log_data;
}

void lrzip_log_stdout_set(Lrzip *lr, FILE *out)
{
	if (!lr) return;
	lr->control->msgout = out;
}

FILE *lrzip_log_stdout_get(Lrzip *lr)
{
	if (!lr) return NULL;
	return lr->control->msgout;
}

void lrzip_log_stderr_set(Lrzip *lr, FILE *err)
{
	if (!lr) return;
	lr->control->msgerr = err;
}

FILE *lrzip_log_stderr_get(Lrzip *lr)
{
	if (!lr) return NULL;
	return lr->control->msgerr;
}

void lrzip_pass_cb_set(Lrzip *lr, Lrzip_Password_Cb cb, void *data)
{
	if (!lr) return;
	lr->control->pass_cb = (void*)cb;
	lr->control->pass_data = data;
}

void lrzip_info_cb_set(Lrzip *lr, Lrzip_Info_Cb cb, void *data)
{
	if (!lr) return;
	lr->control->info_cb = (void*)cb;
	lr->control->info_data = data;
}

