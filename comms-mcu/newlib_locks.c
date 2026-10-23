/* The selected Arduino-package newlib expects retargetable lock hooks. This
 * firmware runs one core and does not call allocation/stdio from IRQ context. */
#include <sys/lock.h>

_LOCK_RECURSIVE_T __lock___malloc_recursive_mutex;

void __retarget_lock_init(_LOCK_T lock) { (void)lock; }
void __retarget_lock_init_recursive(_LOCK_T lock) { (void)lock; }
void __retarget_lock_close(_LOCK_T lock) { (void)lock; }
void __retarget_lock_close_recursive(_LOCK_T lock) { (void)lock; }
void __retarget_lock_acquire(_LOCK_T lock) { (void)lock; }
void __retarget_lock_acquire_recursive(_LOCK_T lock) { (void)lock; }
int __retarget_lock_try_acquire(_LOCK_T lock) { (void)lock; return 1; }
int __retarget_lock_try_acquire_recursive(_LOCK_T lock) { (void)lock; return 1; }
void __retarget_lock_release(_LOCK_T lock) { (void)lock; }
void __retarget_lock_release_recursive(_LOCK_T lock) { (void)lock; }
