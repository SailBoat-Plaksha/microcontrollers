#include <sys/lock.h>
_LOCK_RECURSIVE_T __lock___malloc_recursive_mutex;
void __retarget_lock_init(_LOCK_T l){(void)l;} void __retarget_lock_init_recursive(_LOCK_T l){(void)l;}
void __retarget_lock_close(_LOCK_T l){(void)l;} void __retarget_lock_close_recursive(_LOCK_T l){(void)l;}
void __retarget_lock_acquire(_LOCK_T l){(void)l;} void __retarget_lock_acquire_recursive(_LOCK_T l){(void)l;}
int __retarget_lock_try_acquire(_LOCK_T l){(void)l;return 1;} int __retarget_lock_try_acquire_recursive(_LOCK_T l){(void)l;return 1;}
void __retarget_lock_release(_LOCK_T l){(void)l;} void __retarget_lock_release_recursive(_LOCK_T l){(void)l;}
