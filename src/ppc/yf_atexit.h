#ifndef _YF_ATEXIT_H_20130204_H
#define _YF_ATEXIT_H_20130204_H
/*
* copyright@: kevin_zhong, mail:qq2000zhong@gmail.com
* time: 20130204-19:18:13
*/

#include <base_struct/yf_core.h>
#include <ppc/yf_header.h>

typedef void (*yf_atexit_handler)(void);


//inner will catch some signals, if you override it, you should call 
//yf_exit_with_sig() in your sig handler
//and caution, you should be very carefull if you pfn have locks, keep no locks just simple
yf_int_t  yf_atexit_handler_add(yf_atexit_handler pfn, yf_log_t* log);

//in normal, you should not call this, unless you catch sig, then you should call this
void  yf_exit_with_sig(int signo);

#endif
