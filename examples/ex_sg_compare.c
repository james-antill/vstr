/* This file tests how effiecent it is to hand off data in chunks as against as
 * one big pile
 * -- vstr is kind of built on the idea that this isn't a problem */

#define VSTR_COMPILE_INCLUDE 1

#include <vstr.h>

#ifndef TSTSG_USE_MD5 
# define TSTSG_USE_MD5 0
#endif

#ifndef TSTSG_SZ /* chunk size for custom Vstr */
# define TSTSG_SZ 128
#endif

#ifndef TSTSG_NUM /* number of times to perform each test */
# define TSTSG_NUM (100)
#endif

#if TSTSG_USE_MD5
/* do md5 test ... */
#include <openssl/md5.h>

# define TSTSG_CTX     MD5_CTX
# define TSTSG_INIT    MD5_Init
# define TSTSG_UPDATE  MD5_Update
# define TSTSG_FINI    MD5_Final
# define TSTSG_OUT_LEN MD5_DIGEST_LENGTH
# define TSTSG_NAME    "MD5"
#else
/* do sha1 test .. */
#include <openssl/sha.h>

# define TSTSG_CTX     SHA_CTX
# define TSTSG_INIT    SHA_Init
# define TSTSG_UPDATE  SHA_Update
# define TSTSG_FINI    SHA_Final
# define TSTSG_OUT_LEN SHA_DIGEST_LENGTH
# define TSTSG_NAME    "SHA"
#endif


#define TST_BEG() { \
		struct timeval tv_beg; \
		struct timeval tv_end; \
		struct timeval tv_dif; \
		unsigned int test_for = TSTSG_NUM; \
                unsigned int tst_count = 0; \
                 \
		gettimeofday(&tv_beg, NULL); \
		 \
		while (tst_count < test_for) \
		{ \
			TSTSG_CTX ctx; \
			unsigned char out[TSTSG_OUT_LEN]; \
                  \
        		TSTSG_INIT(&ctx)
             
#define TST_END(name) \
        		TSTSG_FINI(out, &ctx); \
                        ++tst_count; \
		} \
                 \
		gettimeofday(&tv_end, NULL); \
                 \
                 \
                tv_beg.tv_sec -= tstsg_beg.tv_sec; \
                tv_end.tv_sec -= tstsg_beg.tv_sec; \
                 \
                if (tv_end.tv_usec < tv_beg.tv_usec) { \
                  --tv_end.tv_sec; \
                  tv_end.tv_usec += (1000 * 1000); } \
                 \
		tv_dif.tv_sec  = tv_end.tv_sec  - tv_beg.tv_sec; \
		tv_dif.tv_usec = tv_end.tv_usec - tv_beg.tv_usec; \
                 \
		vstr_add_fmt(out, out->len, \
                             "|%16s: %8lu.%-7lu - %8lu.%-7lu = %8lu.%-7lu%s", \
                             (name), \
 		             tv_end.tv_sec, tv_end.tv_usec, \
		             tv_beg.tv_sec, tv_beg.tv_usec, \
		             tv_dif.tv_sec, tv_dif.tv_usec, "  |\n"); \
	}
   

#include <sys/time.h> /* gettimeofday */
#include <assert.h>



int main(int argc, char *argv[])
{
	Vstr_conf *conf = NULL;
	Vstr_base *out = NULL;
	Vstr_base *s1 = NULL;
	Vstr_base *s2 = NULL;
	Vstr_base *s3 = NULL;
	unsigned int err = 0;
        struct timeval tstsg_beg;

        gettimeofday(&tstsg_beg, NULL); \

	if (!vstr_init())
		exit(EXIT_FAILURE);

	conf = vstr_make_conf();
	if (!conf)
		exit(EXIT_FAILURE);

        /* have a custom config. for output and s1 ... */
	vstr_cntl_conf(conf, VSTR_CNTL_CONF_SET_NUM_BUF_SZ, TSTSG_SZ);
        
	vstr_cntl_conf(conf, VSTR_CNTL_CONF_SET_LOC_CSTR_THOU_SEP, "_");
	vstr_cntl_conf(conf, VSTR_CNTL_CONF_SET_LOC_CSTR_THOU_GRP, "\3");
        
        vstr_cntl_conf(conf, VSTR_CNTL_CONF_SET_FMT_CHAR_ESC, '$');
        vstr_sc_fmt_add_bkmg_Byte_uint(conf, "{BKMG:%u}");

        
	out = vstr_make_base(conf); /* used as stdio/stdout */
        
	s1 = vstr_make_base(conf); /* configured size */
	s2 = vstr_make_base(NULL); /* mmap */
	s3 = vstr_make_base(NULL); /* default size */
        vstr_free_conf(conf);
	if (!out || !s1 || !s2 || !s3)
		exit(EXIT_FAILURE);
        
	if (argc != 2)
        {
          vstr_add_fmt(out, out->len, 
                       " Format: %s <filename>\n", argv[0]);
          goto failed;
        }

	vstr_sc_mmap_file(s2, s2->len, argv[1], 0, 0, &err);
        if (err)
          exit (EXIT_FAILURE);

        vstr_add_vstr(s3, s1->len, s2, 1, s2->len, VSTR_TYPE_ADD_ALL_BUF);
        vstr_add_vstr(s1, s1->len, s2, 1, s2->len, VSTR_TYPE_ADD_ALL_BUF);

        if (conf->malloc_bad)
          exit (EXIT_FAILURE);

        /* with I/O the iovec would be valid */
        vstr_export_iovec_ptr_all(s1, NULL, NULL);
        vstr_export_iovec_ptr_all(s3, NULL, NULL);

        /* print table to say what config we are using ... */
        vstr_add_rep_chr(out, out->len, '-', (19 * 4));
        VSTR_ADD_CSTR_BUF(out, out->len, "\n");
        vstr_add_fmt(out, out->len, "|%16s | %16s | %16s | %16s |\n",
                     "name", "len", "chunks", "chunk size");
        
        vstr_add_rep_chr(out, out->len, '-', (19 * 4));
        VSTR_ADD_CSTR_BUF(out, out->len, "\n");        
        {
          unsigned int sz = 0;

          vstr_cntl_conf(s1->conf, VSTR_CNTL_CONF_GET_NUM_BUF_SZ, &sz);
          vstr_add_fmt(out, out->len,
                       "|%16s | $16{BKMG:%u} | %'16zu | $16{BKMG:%u} |\n",
                       "s1: large iovs", (unsigned int)s1->len, s1->num, sz);
          
          vstr_add_fmt(out, out->len,
                       "|%16s | $16{BKMG:%u} | %'16zu | %16s |\n",
                       "s2:       mmap", (unsigned int)s2->len, s2->num, "N/A");
          
          vstr_cntl_conf(s3->conf, VSTR_CNTL_CONF_GET_NUM_BUF_SZ, &sz);
          vstr_add_fmt(out, out->len,
                       "|%16s | $16{BKMG:%u} | %'16zu | $16{BKMG:%u} |\n",
                       "s3: small iovs", (unsigned int)s3->len, s3->num, sz);
        }
        vstr_add_rep_chr(out, out->len, '-', (19 * 4));
        VSTR_ADD_CSTR_BUF(out, out->len, "\n");
        vstr_add_fmt(out, out->len, " Doing tests %'u times with %s:\n",
                     (unsigned int)TSTSG_NUM, TSTSG_NAME);

        while (out->len && !err)
          vstr_sc_write_fd(out, 1, out->len, 1 /* stdout */, &err);

        vstr_add_rep_chr(out, out->len, '-', (19 * 4));
        VSTR_ADD_CSTR_BUF(out, out->len, "\n");
        
	TST_BEG();        
        {  
          struct iovec *iovs = NULL;
          unsigned int count = 0;
          unsigned int num = 0;
          size_t len = 0;
          
          len = vstr_export_iovec_ptr_all(s1, &iovs, &num);

          while (count < num)
          {
            TSTSG_UPDATE(&ctx, iovs[count].iov_base, iovs[count].iov_len);
            ++count;
          }
          
        }
	TST_END("s1: large iovs");
        
        TST_BEG();
        {  
          Vstr_ref *ref = NULL;
          size_t off = 0;
          
          ref = vstr_export_ref(s2, 1, s2->len, &off);
          assert(off == 0);
          
          TSTSG_UPDATE(&ctx, ref->ptr, s1->len);
        }
	TST_END("s2:       mmap");

        vstr_export_iovec_ptr_all(s3, NULL, NULL);
	TST_BEG();
        {
          struct iovec *iovs = NULL;
          unsigned int count = 0;
          unsigned int num = 0;
          size_t len = 0;
          
          len = vstr_export_iovec_ptr_all(s3, &iovs, &num);

          while (count < num)
          {
            TSTSG_UPDATE(&ctx, iovs[count].iov_base, iovs[count].iov_len);
            ++count;
          } 
        }
	TST_END("s3: small iovs");

        vstr_add_rep_chr(out, out->len, '-', (19 * 4));
        VSTR_ADD_CSTR_BUF(out, out->len, "\n");
        
        while (out->len && !err)
          vstr_sc_write_fd(out, 1, out->len, 1 /* stdout */, &err);
        
        vstr_free_base(s1);
        vstr_free_base(s2);
        vstr_free_base(s3);
        
	vstr_exit();

        exit (EXIT_SUCCESS);

 failed:
        while (out->len && !err)
          vstr_sc_write_fd(out, 1, out->len, 2 /* stderr */, &err);
        
        vstr_free_base(s1);
        vstr_free_base(s2);
        vstr_free_base(s3);
        
	vstr_exit();
        
        exit (EXIT_SUCCESS);        
}

