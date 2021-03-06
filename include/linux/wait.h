#ifndef _LINUX_WAIT_H
#define _LINUX_WAIT_H
/*
 * Linux wait queue related types and methods
 */
#include <linux/list.h>
#include <linux/stddef.h>
#include <linux/spinlock.h>
#include <asm/current.h>
#include <uapi/linux/wait.h>

typedef struct wait_queue_entry wait_queue_entry_t;
typedef int (*wait_queue_func_t)(wait_queue_entry_t *wait, unsigned mode, int flags, void *key);
int default_wake_function(wait_queue_entry_t *wait, unsigned mode, int flags, void *key);

/* wait_queue_entry::flags */
#define WQ_FLAG_EXCLUSIVE	0x01
#define WQ_FLAG_WOKEN		0x02

/*
 * A single wait-queue entry structure:
 */
struct wait_queue_entry {
	unsigned int		flags;
	void			*private;
	wait_queue_func_t	func;
	struct list_head	entry;
};

struct wait_bit_key {
	void			*flags;
	int			bit_nr;
#define WAIT_ATOMIC_T_BIT_NR	-1
	unsigned long		timeout;
};

struct wait_bit_queue {
	struct wait_bit_key	key;
	wait_queue_entry_t	wait;
};

struct wait_queue_head {
	spinlock_t		lock;
	struct list_head	head;
};
typedef struct wait_queue_head wait_queue_head_t;

struct task_struct;

/*
 * Macros for declaration and initialisaton of the datatypes
 */

#define __WAITQUEUE_INITIALIZER(name, tsk) {					\
	.private	= tsk,							\
	.func		= default_wake_function,				\
	.entry		= { NULL, NULL } }

#define DECLARE_WAITQUEUE(name, tsk)					\
	wait_queue_entry_t name = __WAITQUEUE_INITIALIZER(name, tsk)

#define __WAIT_QUEUE_HEAD_INITIALIZER(name) {					\
	.lock		= __SPIN_LOCK_UNLOCKED(name.lock),			\
	.head		= { &(name).head, &(name).head } }

#define DECLARE_WAIT_QUEUE_HEAD(name) \
	struct wait_queue_head name = __WAIT_QUEUE_HEAD_INITIALIZER(name)

#define __WAIT_BIT_KEY_INITIALIZER(word, bit)				\
	{ .flags = word, .bit_nr = bit, }

#define __WAIT_ATOMIC_T_KEY_INITIALIZER(p)				\
	{ .flags = p, .bit_nr = WAIT_ATOMIC_T_BIT_NR, }

extern void __init_waitqueue_head(struct wait_queue_head *wq_head, const char *name, struct lock_class_key *);

#define init_waitqueue_head(wq_head)			\
	do {						\
		static struct lock_class_key __key;	\
							\
		__init_waitqueue_head((wq_head), #wq_head, &__key);	\
	} while (0)

#ifdef CONFIG_LOCKDEP
# define __WAIT_QUEUE_HEAD_INIT_ONSTACK(name) \
	({ init_waitqueue_head(&name); name; })
# define DECLARE_WAIT_QUEUE_HEAD_ONSTACK(name) \
	struct wait_queue_head name = __WAIT_QUEUE_HEAD_INIT_ONSTACK(name)
#else
# define DECLARE_WAIT_QUEUE_HEAD_ONSTACK(name) DECLARE_WAIT_QUEUE_HEAD(name)
#endif

static inline void init_waitqueue_entry(wait_queue_entry_t *q, struct task_struct *p)
{
	q->flags	= 0;
	q->private	= p;
	q->func		= default_wake_function;
}

static inline void
init_waitqueue_func_entry(wait_queue_entry_t *q, wait_queue_func_t func)
{
	q->flags	= 0;
	q->private	= NULL;
	q->func		= func;
}

/*
 * Used for wake-one threads:
 */


typedef int wait_bit_action_f(struct wait_bit_key *, int mode);
void __wake_up(struct wait_queue_head *wq_head, unsigned int mode, int nr, void *key);
void __wake_up_locked_key(struct wait_queue_head *wq_head, unsigned int mode, void *key);
void __wake_up_sync_key(struct wait_queue_head *wq_head, unsigned int mode, int nr, void *key);
void __wake_up_locked(struct wait_queue_head *wq_head, unsigned int mode, int nr);
void __wake_up_sync(struct wait_queue_head *wq_head, unsigned int mode, int nr);
void __wake_up_bit(struct wait_queue_head *, void *, int);
int __wait_on_bit(struct wait_queue_head *, struct wait_bit_queue *, wait_bit_action_f *, unsigned);
int __wait_on_bit_lock(struct wait_queue_head *, struct wait_bit_queue *, wait_bit_action_f *, unsigned);
void wake_up_bit(void *, int);
void wake_up_atomic_t(atomic_t *);
int out_of_line_wait_on_bit(void *, int, wait_bit_action_f *, unsigned);
int out_of_line_wait_on_bit_timeout(void *, int, wait_bit_action_f *, unsigned, unsigned long);
int out_of_line_wait_on_bit_lock(void *, int, wait_bit_action_f *, unsigned);
int out_of_line_wait_on_atomic_t(atomic_t *, int (*)(atomic_t *), unsigned);
struct wait_queue_head *bit_waitqueue(void *, int);

#define wake_up(x)			__wake_up(x, TASK_NORMAL, 1, NULL)
#define wake_up_nr(x, nr)		__wake_up(x, TASK_NORMAL, nr, NULL)
#define wake_up_all(x)			__wake_up(x, TASK_NORMAL, 0, NULL)
#define wake_up_locked(x)		__wake_up_locked((x), TASK_NORMAL, 1)
#define wake_up_all_locked(x)		__wake_up_locked((x), TASK_NORMAL, 0)

#define wake_up_interruptible(x)	__wake_up(x, TASK_INTERRUPTIBLE, 1, NULL)
#define wake_up_interruptible_nr(x, nr)	__wake_up(x, TASK_INTERRUPTIBLE, nr, NULL)
#define wake_up_interruptible_all(x)	__wake_up(x, TASK_INTERRUPTIBLE, 0, NULL)
#define wake_up_interruptible_sync(x)	__wake_up_sync((x), TASK_INTERRUPTIBLE, 1)

/*
 * Wakeup macros to be used to report events to the targets.
 */
#define wake_up_poll(x, m)						\
	__wake_up(x, TASK_NORMAL, 1, (void *) (m))
#define wake_up_locked_poll(x, m)					\
	__wake_up_locked_key((x), TASK_NORMAL, (void *) (m))
#define wake_up_interruptible_poll(x, m)				\
	__wake_up(x, TASK_INTERRUPTIBLE, 1, (void *) (m))
#define wake_up_interruptible_sync_poll(x, m)				\
	__wake_up_sync_key((x), TASK_INTERRUPTIBLE, 1, (void *) (m))

#define ___wait_cond_timeout(condition)					\
({									\
	bool __cond = (condition);					\
	if (__cond && !__ret)						\
		__ret = 1;						\
	__cond || !__ret;						\
})

#define ___wait_is_interruptible(state)					\
	(!__builtin_constant_p(state) ||				\
		state == TASK_INTERRUPTIBLE || state == TASK_KILLABLE)	\

extern void init_wait_entry(wait_queue_entry_t *__wait, int flags);

/*
 * The below macro ___wait_event() has an explicit shadow of the __ret
 * variable when used from the wait_event_*() macros.
 *
 * This is so that both can use the ___wait_cond_timeout() construct
 * to wrap the condition.
 *
 * The type inconsistency of the wait_event_*() __ret variable is also
 * on purpose; we use long where we can return timeout values and int
 * otherwise.
 */
//#ifdef VENDOR_EDIT //fangpan@Swdp.shanghai,2015/11/12
#define ___wait_event(wq, condition, state, exclusive, ret, cmd)	\
({									\
	__label__ __out;						\
	wait_queue_entry_t __wait;						\
	long __ret = ret;	/* explicit shadow */			\
									\
	init_wait_entry(&__wait, exclusive ? WQ_FLAG_EXCLUSIVE : 0);	\
	for (;;) {							\
		long __int = prepare_to_wait_event(&wq, &__wait, state);\
									\
		if (condition)						\
			break;						\
									\
		if (___wait_is_interruptible(state) && __int) {		\
			__ret = __int;					\
			goto __out;					\
		}							\
									\
		if(hung_long_and_fatal_signal_pending(current)) { 	\
			break;						\
		}							\
		cmd;							\
	}								\
	finish_wait(&wq, &__wait);					\
__out:	__ret;								\
})
//#endif

#define __wait_event(wq, condition)					\
	(void)___wait_event(wq, condition, TASK_UNINTERRUPTIBLE, 0, 0,	\
			    schedule())

/**
 * wait_event - sleep until a condition gets true
 * @wq: the waitqueue to wait on
 * @condition: a C expression for the event to wait for
 *
 * The process is put to sleep (TASK_UNINTERRUPTIBLE) until the
 * @condition evaluates to true. The @condition is checked each time
 * the waitqueue @wq is woken up.
 *
 * wake_up() has to be called after changing any variable that could
 * change the result of the wait condition.
 */
#define wait_event(wq, condition)					\
do {									\
	might_sleep();							\
	if (condition)							\
		break;							\
	__wait_event(wq, condition);					\
} while (0)

#define __io_wait_event(wq, condition)					\
	(void)___wait_event(wq, condition, TASK_UNINTERRUPTIBLE, 0, 0,	\
			    io_schedule())

/*
 * io_wait_event() -- like wait_event() but with io_schedule()
 */
#define io_wait_event(wq, condition)					\
do {									\
	might_sleep();							\
	if (condition)							\
		break;							\
	__io_wait_event(wq, condition);					\
} while (0)

#define __wait_event_freezable(wq, condition)				\
	___wait_event(wq, condition, TASK_INTERRUPTIBLE, 0, 0,		\
			    schedule(); try_to_freeze())

/**
 * wait_event_freezable - sleep (or freeze) until a condition gets true
 * @wq: the waitqueue to wait on
 * @condition: a C expression for the event to wait for
 *
 * The process is put to sleep (TASK_INTERRUPTIBLE -- so as not to contribute
 * to system load) until the @condition evaluates to true. The
 * @condition is checked each time the waitqueue @wq is woken up.
 *
 * wake_up() has to be called after changing any variable that could
 * change the result of the wait condition.
 */
#define wait_event_freezable(wq, condition)				\
({									\
	int __ret = 0;							\
	might_sleep();							\
	if (!(condition))						\
		__ret = __wait_event_freezable(wq, condition);		\
	__ret;								\
})

#define __wait_event_timeout(wq, condition, timeout)			\
	___wait_event(wq, ___wait_cond_timeout(condition),		\
		      TASK_UNINTERRUPTIBLE, 0, timeout,			\
		      __ret = schedule_timeout(__ret))

/**
 * wait_event_timeout - sleep until a condition gets true or a timeout elapses
 * @wq: the waitqueue to wait on
 * @condition: a C expression for the event to wait for
 * @timeout: timeout, in jiffies
 *
 * The process is put to sleep (TASK_UNINTERRUPTIBLE) until the
 * @condition evaluates to true. The @condition is checked each time
 * the waitqueue @wq is woken up.
 *
 * wake_up() has to be called after changing any variable that could
 * change the result of the wait condition.
 *
 * Returns:
 * 0 if the @condition evaluated to %false after the @timeout elapsed,
 * 1 if the @condition evaluated to %true after the @timeout elapsed,
 * or the remaining jiffies (at least 1) if the @condition evaluated
 * to %true before the @timeout elapsed.
 */
#define wait_event_timeout(wq, condition, timeout)			\
({									\
	long __ret = timeout;						\
	might_sleep();							\
	if (!___wait_cond_timeout(condition))				\
		__ret = __wait_event_timeout(wq, condition, timeout);	\
	__ret;								\
})

#define __wait_event_freezable_timeout(wq, condition, timeout)		\
	___wait_event(wq, ___wait_cond_timeout(condition),		\
		      TASK_INTERRUPTIBLE, 0, timeout,			\
		      __ret = schedule_timeout(__ret); try_to_freeze())

/*
 * like wait_event_timeout() -- except it uses TASK_INTERRUPTIBLE to avoid
 * increasing load and is freezable.
 */
#define wait_event_freezable_timeout(wq, condition, timeout)		\
({									\
	long __ret = timeout;						\
	might_sleep();							\
	if (!___wait_cond_timeout(condition))				\
		__ret = __wait_event_freezable_timeout(wq, condition, timeout);	\
	__ret;								\
})

#define __wait_event_exclusive_cmd(wq, condition, cmd1, cmd2)		\
	(void)___wait_event(wq, condition, TASK_UNINTERRUPTIBLE, 1, 0,	\
			    cmd1; schedule(); cmd2)
/*
 * Just like wait_event_cmd(), except it sets exclusive flag
 */
#define wait_event_exclusive_cmd(wq, condition, cmd1, cmd2)		\
do {									\
	if (condition)							\
		break;							\
	__wait_event_exclusive_cmd(wq, condition, cmd1, cmd2);		\
} while (0)

#define __wait_event_cmd(wq, condition, cmd1, cmd2)			\
	(void)___wait_event(wq, condition, TASK_UNINTERRUPTIBLE, 0, 0,	\
			    cmd1; schedule(); cmd2)

/**
 * wait_event_cmd - sleep until a condition gets true
 * @wq: the waitqueue to wait on
 * @condition: a C expression for the event to wait for
 * @cmd1: the command will be executed before sleep
 * @cmd2: the command will be executed after sleep
 *
 * The process is put to sleep (TASK_UNINTERRUPTIBLE) until the
 * @condition evaluates to true. The @condition is checked each time
 * the waitqueue @wq is woken up.
 *
 * wake_up() has to be called after changing any variable that could
 * change the result of the wait condition.
 */
#define wait_event_cmd(wq, condition, cmd1, cmd2)			\
do {									\
	if (condition)							\
		break;							\
	__wait_event_cmd(wq, condition, cmd1, cmd2);			\
} while (0)

#define __wait_event_interruptible(wq, condition)			\
	___wait_event(wq, condition, TASK_INTERRUPTIBLE, 0, 0,		\
		      schedule())

/**
 * wait_event_interruptible - sleep until a condition gets true
 * @wq: the waitqueue to wait on
 * @condition: a C expression for the event to wait for
 *
 * The process is put to sleep (TASK_INTERRUPTIBLE) until the
 * @condition evaluates to true or a signal is received.
 * The @condition is checked each time the waitqueue @wq is woken up.
 *
 * wake_up() has to be called after changing any variable that could
 * change the result of the wait condition.
 *
 * The function will return -ERESTARTSYS if it was interrupted by a
 * signal and 0 if @condition evaluated to true.
 */
#define wait_event_interruptible(wq, condition)				\
({									\
	int __ret = 0;							\
	might_sleep();							\
	if (!(condition))						\
		__ret = __wait_event_interruptible(wq, condition);	\
	__ret;								\
})

#define __wait_event_interruptible_timeout(wq, condition, timeout)	\
	___wait_event(wq, ___wait_cond_timeout(condition),		\
		      TASK_INTERRUPTIBLE, 0, timeout,			\
		      __ret = schedule_timeout(__ret))

/**
 * wait_event_interruptible_timeout - sleep until a condition gets true or a timeout elapses
 * @wq: the waitqueue to wait on
 * @condition: a C expression for the event to wait for
 * @timeout: timeout, in jiffies
 *
 * The process is put to sleep (TASK_INTERRUPTIBLE) until the
 * @condition evaluates to true or a signal is received.
 * The @condition is checked each time the waitqueue @wq is woken up.
 *
 * wake_up() has to be called after changing any variable that could
 * change the result of the wait condition.
 *
 * Returns:
 * 0 if the @condition evaluated to %false after the @timeout elapsed,
 * 1 if the @condition evaluated to %true after the @timeout elapsed,
 * the remaining jiffies (at least 1) if the @condition evaluated
 * to %true before the @timeout elapsed, or -%ERESTARTSYS if it was
 * interrupted by a signal.
 */
#define wait_event_interruptible_timeout(wq, condition, timeout)	\
({									\
	long __ret = timeout;						\
	might_sleep();							\
	if (!___wait_cond_timeout(condition))				\
		__ret = __wait_event_interruptible_timeout(wq,		\
						condition, timeout);	\
	__ret;								\
})

#define __wait_event_hrtimeout(wq, condition, timeout, state)		\
({									\
	int __ret = 0;							\
	struct hrtimer_sleeper __t;					\
									\
	hrtimer_init_on_stack(&__t.timer, CLOCK_MONOTONIC,		\
			      HRTIMER_MODE_REL);			\
	hrtimer_init_sleeper(&__t, current);				\
	if ((timeout).tv64 != KTIME_MAX)				\
		hrtimer_start_range_ns(&__t.timer, timeout,		\
				       current->timer_slack_ns,		\
				       HRTIMER_MODE_REL);		\
									\
	__ret = ___wait_event(wq, condition, state, 0, 0,		\
		if (!__t.task) {					\
			__ret = -ETIME;					\
			break;						\
		}							\
		schedule());						\
									\
	hrtimer_cancel(&__t.timer);					\
	destroy_hrtimer_on_stack(&__t.timer);				\
	__ret;								\
})

/**
 * wait_event_hrtimeout - sleep until a condition gets true or a timeout elapses
 * @wq: the waitqueue to wait on
 * @condition: a C expression for the event to wait for
 * @timeout: timeout, as a ktime_t
 *
 * The process is put to sleep (TASK_UNINTERRUPTIBLE) until the
 * @condition evaluates to true or a signal is received.
 * The @condition is checked each time the waitqueue @wq is woken up.
 *
 * wake_up() has to be called after changing any variable that could
 * change the result of the wait condition.
 *
 * The function returns 0 if @condition became true, or -ETIME if the timeout
 * elapsed.
 */
#define wait_event_hrtimeout(wq, condition, timeout)			\
({									\
	int __ret = 0;							\
	might_sleep();							\
	if (!(condition))						\
		__ret = __wait_event_hrtimeout(wq, condition, timeout,	\
					       TASK_UNINTERRUPTIBLE);	\
	__ret;								\
})

/**
 * wait_event_interruptible_hrtimeout - sleep until a condition gets true or a timeout elapses
 * @wq: the waitqueue to wait on
 * @condition: a C expression for the event to wait for
 * @timeout: timeout, as a ktime_t
 *
 * The process is put to sleep (TASK_INTERRUPTIBLE) until the
 * @condition evaluates to true or a signal is received.
 * The @condition is checked each time the waitqueue @wq is woken up.
 *
 * wake_up() has to be called after changing any variable that could
 * change the result of the wait condition.
 *
 * The function returns 0 if @condition became true, -ERESTARTSYS if it was
 * interrupted by a signal, or -ETIME if the timeout elapsed.
 */
#define wait_event_interruptible_hrtimeout(wq, condition, timeout)	\
({									\
	long __ret = 0;							\
	might_sleep();							\
	if (!(condition))						\
		__ret = __wait_event_hrtimeout(wq, condition, timeout,	\
					       TASK_INTERRUPTIBLE);	\
	__ret;								\
})

#define __wait_event_interruptible_exclusive(wq, condition)		\
	___wait_event(wq, condition, TASK_INTERRUPTIBLE, 1, 0,		\
		      schedule())

#define wait_event_interruptible_exclusive(wq, condition)		\
({									\
	int __ret = 0;							\
	might_sleep();							\
	if (!(condition))						\
		__ret = __wait_event_interruptible_exclusive(wq, condition);\
	__ret;								\
})

#define __wait_event_killable_exclusive(wq, condition)			\
	___wait_event(wq, condition, TASK_KILLABLE, 1, 0,		\
		      schedule())

#define wait_event_killable_exclusive(wq, condition)			\
({									\
	int __ret = 0;							\
	might_sleep();							\
	if (!(condition))						\
		__ret = __wait_event_killable_exclusive(wq, condition);	\
	__ret;								\
})


#define __wait_event_freezable_exclusive(wq, condition)			\
	___wait_event(wq, condition, TASK_INTERRUPTIBLE, 1, 0,		\
			schedule(); try_to_freeze())

#define wait_event_freezable_exclusive(wq, condition)			\
({									\
	int __ret = 0;							\
	might_sleep();							\
	if (!(condition))						\
		__ret = __wait_event_freezable_exclusive(wq, condition);\
	__ret;								\
})

extern int do_wait_intr(wait_queue_head_t *, wait_queue_entry_t *);
extern int do_wait_intr_irq(wait_queue_head_t *, wait_queue_entry_t *);

#define __wait_event_interruptible_locked(wq, condition, exclusive, irq) \
({									\
	int __ret = 0;							\
	DEFINE_WAIT(__wait);						\
	if (exclusive)							\
		__wait.flags |= WQ_FLAG_EXCLUSIVE;			\
	do {								\
		if (likely(list_empty(&__wait.task_list)))		\
			__add_wait_queue_tail(&(wq), &__wait);		\
		set_current_state(TASK_INTERRUPTIBLE);			\
		if (signal_pending(current)) {				\
			__ret = -ERESTARTSYS;				\
			break;						\
		}							\
		if (irq)						\
			spin_unlock_irq(&(wq).lock);			\
		else							\
			spin_unlock(&(wq).lock);			\
		schedule();						\
		if (irq)						\
			spin_lock_irq(&(wq).lock);			\
		else							\
			spin_lock(&(wq).lock);				\
	} while (!(condition));						\
	__remove_wait_queue(&(wq), &__wait);				\
	__set_current_state(TASK_RUNNING);				\
	__ret;								\
})


/**
 * wait_event_interruptible_locked - sleep until a condition gets true
 * @wq: the waitqueue to wait on
 * @condition: a C expression for the event to wait for
 *
 * The process is put to sleep (TASK_INTERRUPTIBLE) until the
 * @condition evaluates to true or a signal is received.
 * The @condition is checked each time the waitqueue @wq is woken up.
 *
 * It must be called with wq.lock being held.  This spinlock is
 * unlocked while sleeping but @condition testing is done while lock
 * is held and when this macro exits the lock is held.
 *
 * The lock is locked/unlocked using spin_lock()/spin_unlock()
 * functions which must match the way they are locked/unlocked outside
 * of this macro.
 *
 * wake_up_locked() has to be called after changing any variable that could
 * change the result of the wait condition.
 *
 * The function will return -ERESTARTSYS if it was interrupted by a
 * signal and 0 if @condition evaluated to true.
 */
#define wait_event_interruptible_locked(wq, condition)			\
	((condition)							\
	 ? 0 : __wait_event_interruptible_locked(wq, condition, 0, 0))

/**
 * wait_event_interruptible_locked_irq - sleep until a condition gets true
 * @wq: the waitqueue to wait on
 * @condition: a C expression for the event to wait for
 *
 * The process is put to sleep (TASK_INTERRUPTIBLE) until the
 * @condition evaluates to true or a signal is received.
 * The @condition is checked each time the waitqueue @wq is woken up.
 *
 * It must be called with wq.lock being held.  This spinlock is
 * unlocked while sleeping but @condition testing is done while lock
 * is held and when this macro exits the lock is held.
 *
 * The lock is locked/unlocked using spin_lock_irq()/spin_unlock_irq()
 * functions which must match the way they are locked/unlocked outside
 * of this macro.
 *
 * wake_up_locked() has to be called after changing any variable that could
 * change the result of the wait condition.
 *
 * The function will return -ERESTARTSYS if it was interrupted by a
 * signal and 0 if @condition evaluated to true.
 */
#define wait_event_interruptible_locked_irq(wq, condition)		\
	((condition)							\
	 ? 0 : __wait_event_interruptible_locked(wq, condition, 0, 1))

/**
 * wait_event_interruptible_exclusive_locked - sleep exclusively until a condition gets true
 * @wq: the waitqueue to wait on
 * @condition: a C expression for the event to wait for
 *
 * The process is put to sleep (TASK_INTERRUPTIBLE) until the
 * @condition evaluates to true or a signal is received.
 * The @condition is checked each time the waitqueue @wq is woken up.
 *
 * It must be called with wq.lock being held.  This spinlock is
 * unlocked while sleeping but @condition testing is done while lock
 * is held and when this macro exits the lock is held.
 *
 * The lock is locked/unlocked using spin_lock()/spin_unlock()
 * functions which must match the way they are locked/unlocked outside
 * of this macro.
 *
 * The process is put on the wait queue with an WQ_FLAG_EXCLUSIVE flag
 * set thus when other process waits process on the list if this
 * process is awaken further processes are not considered.
 *
 * wake_up_locked() has to be called after changing any variable that could
 * change the result of the wait condition.
 *
 * The function will return -ERESTARTSYS if it was interrupted by a
 * signal and 0 if @condition evaluated to true.
 */
#define wait_event_interruptible_exclusive_locked(wq, condition)	\
	((condition)							\
	 ? 0 : __wait_event_interruptible_locked(wq, condition, 1, 0))

/**
 * wait_event_interruptible_exclusive_locked_irq - sleep until a condition gets true
 * @wq: the waitqueue to wait on
 * @condition: a C expression for the event to wait for
 *
 * The process is put to sleep (TASK_INTERRUPTIBLE) until the
 * @condition evaluates to true or a signal is received.
 * The @condition is checked each time the waitqueue @wq is woken up.
 *
 * It must be called with wq.lock being held.  This spinlock is
 * unlocked while sleeping but @condition testing is done while lock
 * is held and when this macro exits the lock is held.
 *
 * The lock is locked/unlocked using spin_lock_irq()/spin_unlock_irq()
 * functions which must match the way they are locked/unlocked outside
 * of this macro.
 *
 * The process is put on the wait queue with an WQ_FLAG_EXCLUSIVE flag
 * set thus when other process waits process on the list if this
 * process is awaken further processes are not considered.
 *
 * wake_up_locked() has to be called after changing any variable that could
 * change the result of the wait condition.
 *
 * The function will return -ERESTARTSYS if it was interrupted by a
 * signal and 0 if @condition evaluated to true.
 */
#define wait_event_interruptible_exclusive_locked_irq(wq, condition)	\
	((condition)							\
	 ? 0 : __wait_event_interruptible_locked(wq, condition, 1, 1))


#define __wait_event_killable(wq, condition)				\
	___wait_event(wq, condition, TASK_KILLABLE, 0, 0, schedule())

/**
 * wait_event_killable - sleep until a condition gets true
 * @wq: the waitqueue to wait on
 * @condition: a C expression for the event to wait for
 *
 * The process is put to sleep (TASK_KILLABLE) until the
 * @condition evaluates to true or a signal is received.
 * The @condition is checked each time the waitqueue @wq is woken up.
 *
 * wake_up() has to be called after changing any variable that could
 * change the result of the wait condition.
 *
 * The function will return -ERESTARTSYS if it was interrupted by a
 * signal and 0 if @condition evaluated to true.
 */
#define wait_event_killable(wq, condition)				\
({									\
	int __ret = 0;							\
	might_sleep();							\
	if (!(condition))						\
		__ret = __wait_event_killable(wq, condition);		\
	__ret;								\
})


#define __wait_event_lock_irq(wq, condition, lock, cmd)			\
	(void)___wait_event(wq, condition, TASK_UNINTERRUPTIBLE, 0, 0,	\
			    spin_unlock_irq(&lock);			\
			    cmd;					\
			    schedule();					\
			    spin_lock_irq(&lock))

/**
 * wait_event_lock_irq_cmd - sleep until a condition gets true. The
 *			     condition is checked under the lock. This
 *			     is expected to be called with the lock
 *			     taken.
 * @wq: the waitqueue to wait on
 * @condition: a C expression for the event to wait for
 * @lock: a locked spinlock_t, which will be released before cmd
 *	  and schedule() and reacquired afterwards.
 * @cmd: a command which is invoked outside the critical section before
 *	 sleep
 *
 * The process is put to sleep (TASK_UNINTERRUPTIBLE) until the
 * @condition evaluates to true. The @condition is checked each time
 * the waitqueue @wq is woken up.
 *
 * wake_up() has to be called after changing any variable that could
 * change the result of the wait condition.
 *
 * This is supposed to be called while holding the lock. The lock is
 * dropped before invoking the cmd and going to sleep and is reacquired
 * afterwards.
 */
#define wait_event_lock_irq_cmd(wq, condition, lock, cmd)		\
do {									\
	if (condition)							\
		break;							\
	__wait_event_lock_irq(wq, condition, lock, cmd);		\
} while (0)

/**
 * wait_event_lock_irq - sleep until a condition gets true. The
 *			 condition is checked under the lock. This
 *			 is expected to be called with the lock
 *			 taken.
 * @wq: the waitqueue to wait on
 * @condition: a C expression for the event to wait for
 * @lock: a locked spinlock_t, which will be released before schedule()
 *	  and reacquired afterwards.
 *
 * The process is put to sleep (TASK_UNINTERRUPTIBLE) until the
 * @condition evaluates to true. The @condition is checked each time
 * the waitqueue @wq is woken up.
 *
 * wake_up() has to be called after changing any variable that could
 * change the result of the wait condition.
 *
 * This is supposed to be called while holding the lock. The lock is
 * dropped before going to sleep and is reacquired afterwards.
 */
#define wait_event_lock_irq(wq, condition, lock)			\
do {									\
	if (condition)							\
		break;							\
	__wait_event_lock_irq(wq, condition, lock, );			\
} while (0)


#define __wait_event_interruptible_lock_irq(wq, condition, lock, cmd)	\
	___wait_event(wq, condition, TASK_INTERRUPTIBLE, 0, 0,		\
		      spin_unlock_irq(&lock);				\
		      cmd;						\
		      schedule();					\
		      spin_lock_irq(&lock))

/**
 * wait_event_interruptible_lock_irq_cmd - sleep until a condition gets true.
 *		The condition is checked under the lock. This is expected to
 *		be called with the lock taken.
 * @wq: the waitqueue to wait on
 * @condition: a C expression for the event to wait for
 * @lock: a locked spinlock_t, which will be released before cmd and
 *	  schedule() and reacquired afterwards.
 * @cmd: a command which is invoked outside the critical section before
 *	 sleep
 *
 * The process is put to sleep (TASK_INTERRUPTIBLE) until the
 * @condition evaluates to true or a signal is received. The @condition is
 * checked each time the waitqueue @wq is woken up.
 *
 * wake_up() has to be called after changing any variable that could
 * change the result of the wait condition.
 *
 * This is supposed to be called while holding the lock. The lock is
 * dropped before invoking the cmd and going to sleep and is reacquired
 * afterwards.
 *
 * The macro will return -ERESTARTSYS if it was interrupted by a signal
 * and 0 if @condition evaluated to true.
 */
#define wait_event_interruptible_lock_irq_cmd(wq, condition, lock, cmd)	\
({									\
	int __ret = 0;							\
	if (!(condition))						\
		__ret = __wait_event_interruptible_lock_irq(wq,		\
						condition, lock, cmd);	\
	__ret;								\
})

/**
 * wait_event_interruptible_lock_irq - sleep until a condition gets true.
 *		The condition is checked under the lock. This is expected
 *		to be called with the lock taken.
 * @wq: the waitqueue to wait on
 * @condition: a C expression for the event to wait for
 * @lock: a locked spinlock_t, which will be released before schedule()
 *	  and reacquired afterwards.
 *
 * The process is put to sleep (TASK_INTERRUPTIBLE) until the
 * @condition evaluates to true or signal is received. The @condition is
 * checked each time the waitqueue @wq is woken up.
 *
 * wake_up() has to be called after changing any variable that could
 * change the result of the wait condition.
 *
 * This is supposed to be called while holding the lock. The lock is
 * dropped before going to sleep and is reacquired afterwards.
 *
 * The macro will return -ERESTARTSYS if it was interrupted by a signal
 * and 0 if @condition evaluated to true.
 */
#define wait_event_interruptible_lock_irq(wq, condition, lock)		\
({									\
	int __ret = 0;							\
	if (!(condition))						\
		__ret = __wait_event_interruptible_lock_irq(wq,		\
						condition, lock,);	\
	__ret;								\
})

#define __wait_event_interruptible_lock_irq_timeout(wq, condition,	\
						    lock, timeout)	\
	___wait_event(wq, ___wait_cond_timeout(condition),		\
		      TASK_INTERRUPTIBLE, 0, timeout,			\
		      spin_unlock_irq(&lock);				\
		      __ret = schedule_timeout(__ret);			\
		      spin_lock_irq(&lock));

/**
 * wait_event_interruptible_lock_irq_timeout - sleep until a condition gets
 *		true or a timeout elapses. The condition is checked under
 *		the lock. This is expected to be called with the lock taken.
 * @wq: the waitqueue to wait on
 * @condition: a C expression for the event to wait for
 * @lock: a locked spinlock_t, which will be released before schedule()
 *	  and reacquired afterwards.
 * @timeout: timeout, in jiffies
 *
 * The process is put to sleep (TASK_INTERRUPTIBLE) until the
 * @condition evaluates to true or signal is received. The @condition is
 * checked each time the waitqueue @wq is woken up.
 *
 * wake_up() has to be called after changing any variable that could
 * change the result of the wait condition.
 *
 * This is supposed to be called while holding the lock. The lock is
 * dropped before going to sleep and is reacquired afterwards.
 *
 * The function returns 0 if the @timeout elapsed, -ERESTARTSYS if it
 * was interrupted by a signal, and the remaining jiffies otherwise
 * if the condition evaluated to true before the timeout elapsed.
 */
#define wait_event_interruptible_lock_irq_timeout(wq, condition, lock,	\
						  timeout)		\
({									\
	long __ret = timeout;						\
	if (!___wait_cond_timeout(condition))				\
		__ret = __wait_event_interruptible_lock_irq_timeout(	\
					wq, condition, lock, timeout);	\
	__ret;								\
})

/*
 * Waitqueues which are removed from the waitqueue_head at wakeup time
 */
void prepare_to_wait(wait_queue_head_t *q, wait_queue_entry_t *wait, int state);
void prepare_to_wait_exclusive(wait_queue_head_t *q, wait_queue_entry_t *wait, int state);
long prepare_to_wait_event(wait_queue_head_t *q, wait_queue_entry_t *wait, int state);
void finish_wait(wait_queue_head_t *q, wait_queue_entry_t *wait);
long wait_woken(wait_queue_entry_t *wait, unsigned mode, long timeout);
int woken_wake_function(wait_queue_entry_t *wait, unsigned mode, int sync, void *key);
int autoremove_wake_function(wait_queue_entry_t *wait, unsigned mode, int sync, void *key);
int wake_bit_function(wait_queue_entry_t *wait, unsigned mode, int sync, void *key);

#define DEFINE_WAIT_FUNC(name, function)				\
	wait_queue_entry_t name = {					\
		.private	= current,				\
		.func		= function,				\
		.task_list	= LIST_HEAD_INIT((name).task_list),	\
	}

#define DEFINE_WAIT(name) DEFINE_WAIT_FUNC(name, autoremove_wake_function)

#define init_wait(wait)								\
	do {									\
		(wait)->private = current;					\
		(wait)->func = autoremove_wake_function;			\
		INIT_LIST_HEAD(&(wait)->entry);					\
		(wait)->flags = 0;						\
	} while (0)

#endif /* _LINUX_WAIT_H */
