
static Vstr_base *x_str = NULL;
static Vstr_base *y_str = NULL;

#define TEST_CMP_STR(x) #x
#define TEST_CMP_XSTR(x) TEST_CMP_STR(x)

 /* this is a bit sick so ... don't try this at home. but it works */
#define TEST_CMP_BEG(x, y) do { \
 x_str = vstr_dup_buf(NULL, (x), (sizeof(x) - 1)); \
 if (!x_str) die(); \
 y_str = vstr_dup_ptr(NULL, (y), (sizeof(y) - 1)); \
 if (!y_str) die(); \
 if (TEST_CMP_FUNC (x_str, 1, x_str->len, y_str, 1, y_str->len)

#define TEST_CMP_END(x, y) ) return (1); \
 vstr_free_base(x_str); \
 vstr_free_base(y_str); \
 } while (FALSE)

#define TEST_CMP_EQ_0(x, y) \
 TEST_CMP_BEG(x, y) != 0 TEST_CMP_END(x, y)
                                
#define TEST_CMP_GT_0(x, y) \
 TEST_CMP_BEG(x, y) <= 0 TEST_CMP_END(x, y); \
 TEST_CMP_BEG(y, x) >= 0 TEST_CMP_END(y, x)

