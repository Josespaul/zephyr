/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "mem_protect.h"

/* Kernel objects */
K_THREAD_STACK_DEFINE(kobject_stack_1, KOBJECT_STACK_SIZE);
K_THREAD_STACK_DEFINE(kobject_stack_2, KOBJECT_STACK_SIZE);
K_THREAD_STACK_DEFINE(kobject_stack_3, KOBJECT_STACK_SIZE);
K_THREAD_STACK_DEFINE(kobject_stack_4, KOBJECT_STACK_SIZE);

K_SEM_DEFINE(kobject_sem, SEMAPHORE_INIT_COUNT, SEMAPHORE_MAX_COUNT);
K_SEM_DEFINE(kobject_public_sem, SEMAPHORE_INIT_COUNT, SEMAPHORE_MAX_COUNT);
K_MUTEX_DEFINE(kobject_mutex);
__kernel struct k_thread kobject_test_4_tid;
__kernel struct k_thread kobject_test_6_tid;
__kernel struct k_thread kobject_test_7_tid;

__kernel struct k_thread kobject_test_9_tid;
__kernel struct k_thread kobject_test_13_tid;
__kernel struct k_thread kobject_test_14_tid;

__kernel struct k_thread kobject_test_reuse_1_tid, kobject_test_reuse_2_tid;
__kernel struct k_thread kobject_test_reuse_3_tid, kobject_test_reuse_4_tid;
__kernel struct k_thread kobject_test_reuse_5_tid, kobject_test_reuse_6_tid;
__kernel struct k_thread kobject_test_reuse_7_tid, kobject_test_reuse_8_tid;

struct k_thread kobject_test_10_tid_uninitialized;

struct k_sem *random_sem_type;
struct k_sem kobject_sem_not_hash_table;
__kernel struct k_sem kobject_sem_no_init_no_access;
__kernel struct k_sem kobject_sem_no_init_access;


/****************************************************************************/
void kobject_user_tc1(void *p1, void *p2, void *p3)
{
	valid_fault = true;
	USERSPACE_BARRIER;

	k_sem_take(random_sem_type, K_FOREVER);
}

/* Test access to a invalid semaphore who's address is NULL. */
void test_kobject_access_grant(void *p1, void *p2, void *p3)
{

	_k_object_init(random_sem_type);
	k_thread_access_grant(k_current_get(),
			      &kobject_sem,
			      &kobject_mutex,
			      random_sem_type, NULL);

	k_thread_user_mode_enter(kobject_user_tc1, NULL, NULL, NULL);

}
/****************************************************************************/
void kobject_user_tc2(void *p1, void *p2, void *p3)
{
	valid_fault = false;
	USERSPACE_BARRIER;

	k_sem_give(&kobject_sem);

	/* should cause a fault */
	valid_fault = true;
	USERSPACE_BARRIER;

	/* typecasting to override compiler warning */
	k_sem_take((struct k_sem *)&kobject_mutex, K_FOREVER);


}

/* Test if a syscall can take a different type of kobject. */
void test_syscall_invalid_kobject(void *p1, void *p2, void *p3)
{
	k_thread_access_grant(k_current_get(),
			      &kobject_sem,
			      &kobject_mutex, NULL);

	k_thread_user_mode_enter(kobject_user_tc2, NULL, NULL, NULL);

}

/****************************************************************************/
void kobject_user_tc3(void *p1, void *p2, void *p3)
{
	/* should cause a fault */
	valid_fault = true;
	USERSPACE_BARRIER;
	k_sem_give(&kobject_sem);
}

/* test if a user thread can access a k object that has not be granted to it.*/
void test_thread_without_kobject_permission(void *p1, void *p2, void *p3)
{
	k_thread_access_grant(k_current_get(),
			      &kobject_mutex, NULL);

	k_thread_user_mode_enter(kobject_user_tc3, NULL, NULL, NULL);

}

/****************************************************************************/
void kobject_user_test4(void *p1, void *p2, void *p3)
{
	/* should cause a fault */
	if ((u32_t)p1 == 1) {
		valid_fault = false;
	} else {
		valid_fault = true;
	}
	USERSPACE_BARRIER;
	k_sem_give(&kobject_sem);
}

/* Test access revoke. */
void test_kobject_revoke_access(void *p1, void *p2, void *p3)
{
	k_thread_access_grant(k_current_get(),
			      &kobject_sem, NULL);

	k_thread_create(&kobject_test_4_tid,
			kobject_stack_1,
			KOBJECT_STACK_SIZE,
			kobject_user_test4,
			(void *)1, NULL, NULL,
			0, K_INHERIT_PERMS | K_USER, K_NO_WAIT);


	k_sem_take(&sync_sem, MSEC(100));
	k_object_access_revoke(&kobject_sem, k_current_get());

	k_thread_create(&kobject_test_4_tid,
			kobject_stack_1,
			KOBJECT_STACK_SIZE,
			kobject_user_test4,
			(void *)2, NULL, NULL,
			0, K_INHERIT_PERMS | K_USER, K_NO_WAIT);

	k_thread_abort(k_current_get());

}

/****************************************************************************/
/* grant access to all user threads that follow */
void kobject_user_1_test5(void *p1, void *p2, void *p3)
{
	valid_fault = false;
	USERSPACE_BARRIER;

	k_sem_give(&kobject_sem);
	k_object_access_grant(&kobject_sem, &kobject_test_reuse_2_tid);
}

void kobject_user_2_test5(void *p1, void *p2, void *p3)
{
	valid_fault = false;
	USERSPACE_BARRIER;

	k_sem_take(&kobject_sem, K_FOREVER);
	ztest_test_pass();
}

/* Test grant access. Will grant access to another thread for the
 * semaphore it holds.
 */
void test_kobject_grant_access_kobj(void *p1, void *p2, void *p3)
{
	k_thread_access_grant(k_current_get(),
			      &kobject_sem, &kobject_test_reuse_2_tid, NULL);

	k_thread_create(&kobject_test_reuse_1_tid,
			kobject_stack_1,
			KOBJECT_STACK_SIZE,
			kobject_user_1_test5,
			NULL, NULL, NULL,
			0, K_INHERIT_PERMS | K_USER, K_NO_WAIT);


	k_sem_take(&sync_sem, MSEC(100));

	k_thread_create(&kobject_test_reuse_2_tid,
			kobject_stack_2,
			KOBJECT_STACK_SIZE,
			kobject_user_2_test5,
			NULL, NULL, NULL,
			0, K_INHERIT_PERMS|K_USER, K_NO_WAIT);

	k_sem_take(&sync_sem, SYNC_SEM_TIMEOUT);

}

/****************************************************************************/
void kobject_user_test6(void *p1, void *p2, void *p3)
{
	valid_fault = false;
	USERSPACE_BARRIER;

	k_sem_give(&kobject_sem);

	valid_fault = true;
	USERSPACE_BARRIER;

	k_object_access_grant(&kobject_sem, &kobject_test_reuse_2_tid);
	zassert_unreachable("k_object validation  failure");
}

/* Test access grant to thread B from thread A which doesn't have
 * required permissions.
 */
void test_kobject_grant_access_kobj_invalid(void *p1, void *p2, void *p3)
{
	k_thread_access_grant(k_current_get(),
			      &kobject_sem, NULL);

	k_thread_create(&kobject_test_6_tid,
			kobject_stack_3,
			KOBJECT_STACK_SIZE,
			kobject_user_test6,
			NULL, NULL, NULL,
			0, K_INHERIT_PERMS|K_USER, K_NO_WAIT);


	k_sem_take(&sync_sem, SYNC_SEM_TIMEOUT);

}

/****************************************************************************/
void kobject_user_test7(void *p1, void *p2, void *p3)
{
	valid_fault = false;
	USERSPACE_BARRIER;

	k_sem_give(&kobject_sem);
	k_object_release(&kobject_sem);

	valid_fault = true;
	USERSPACE_BARRIER;

	k_sem_give(&kobject_sem);
}

/* Test revoke permission of a k object from userspace. */
void test_kobject_release_from_user(void *p1, void *p2, void *p3)
{
	k_thread_access_grant(k_current_get(),
			      &kobject_sem, NULL);

	k_thread_create(&kobject_test_7_tid,
			kobject_stack_1,
			KOBJECT_STACK_SIZE,
			kobject_user_test7,
			NULL, NULL, NULL,
			0, K_INHERIT_PERMS|K_USER, K_NO_WAIT);

	k_sem_take(&sync_sem, SYNC_SEM_TIMEOUT);

}
/****************************************************************************/
void kobject_user_1_test8(void *p1, void *p2, void *p3)
{
	valid_fault = false;
	USERSPACE_BARRIER;

	k_sem_give(&kobject_public_sem);

}

void kobject_user_2_test8(void *p1, void *p2, void *p3)
{
	valid_fault = false;
	USERSPACE_BARRIER;

	k_sem_take(&kobject_public_sem, K_FOREVER);
	ztest_test_pass();
}

/* Test all access grant. test the access by creating 2 new user threads.
 */
void test_kobject_access_all_grant(void *p1, void *p2, void *p3)
{

	k_object_access_all_grant(&kobject_public_sem);
	k_thread_create(&kobject_test_reuse_1_tid,
			kobject_stack_3,
			KOBJECT_STACK_SIZE,
			kobject_user_1_test8,
			NULL, NULL, NULL,
			0, K_USER, K_NO_WAIT);

	k_sem_take(&sync_sem, MSEC(100));

	k_thread_create(&kobject_test_reuse_2_tid,
			kobject_stack_4,
			KOBJECT_STACK_SIZE,
			kobject_user_2_test8,
			NULL, NULL, NULL,
			0, K_USER, K_NO_WAIT);

	k_sem_take(&sync_sem, SYNC_SEM_TIMEOUT);

}
/****************************************************************************/

void kobject_user_1_test9(void *p1, void *p2, void *p3)
{
	valid_fault = false;
	USERSPACE_BARRIER;

	k_sem_give(&kobject_sem);
	k_thread_abort(k_current_get());

}

void kobject_user_2_test9(void *p1, void *p2, void *p3)
{

	valid_fault = true;
	USERSPACE_BARRIER;

	k_sem_take(&kobject_sem, K_FOREVER);
	zassert_unreachable("Failed to clear premission on a deleted thread");
}

/* if a deleted thread with some permissions is recreated with the same tid
 * Check if it still has the permissions.
 */
void test_thread_has_residual_permissions(void *p1, void *p2, void *p3)
{

	k_thread_access_grant(k_current_get(),
			      &kobject_sem, NULL);

	k_thread_create(&kobject_test_9_tid,
			kobject_stack_1,
			KOBJECT_STACK_SIZE,
			kobject_user_1_test9,
			NULL, NULL, NULL,
			0, K_INHERIT_PERMS | K_USER, K_NO_WAIT);


	k_sem_take(&sync_sem, MSEC(100));

	k_thread_create(&kobject_test_9_tid,
			kobject_stack_1,
			KOBJECT_STACK_SIZE,
			kobject_user_2_test9,
			NULL, NULL, NULL,
			0, K_USER, K_NO_WAIT);


	k_sem_take(&sync_sem, SYNC_SEM_TIMEOUT);
}

/****************************************************************************/
/*grant access to a valid kobject but invalid thread id */
#define ERROR_STR_TEST_10 "Access granted/revoked to invalid thread k_object"
void test_kobject_access_grant_to_invalid_thread(void *p1, void *p2, void *p3)
{

	k_object_access_grant(&kobject_sem, &kobject_test_10_tid_uninitialized);
	k_object_access_revoke(&kobject_sem,
			       &kobject_test_10_tid_uninitialized);

	/* Test if this has actually taken the required branch */
	extern void *_k_object_find(void *object);
	void *ret_value = _k_object_find(&kobject_test_10_tid_uninitialized);

	if (ret_value == NULL) {
		ztest_test_pass();
	} else {
		zassert_unreachable(ERROR_STR_TEST_10);
	}
}
/****************************************************************************/
/* object validation checks */
/* Test syscall on a kobject which is not present in the hash table. */
void test_kobject_access_invalid_kobject(void *p1, void *p2, void *p3)
{
	valid_fault = true;
	USERSPACE_BARRIER;

	k_sem_take(&kobject_sem_not_hash_table, K_SECONDS(1));
	zassert_unreachable("k_object validation  failure.");

}

/****************************************************************************/
/* object validation checks */
/* Test syscall on a kobject which is not initialized and has no access */
void test_access_kobject_without_init_access(void *p1,
					     void *p2, void *p3)
{
	valid_fault = true;
	USERSPACE_BARRIER;

	k_sem_take(&kobject_sem_no_init_no_access, K_SECONDS(1));
	zassert_unreachable("k_object validation  failure");

}
/****************************************************************************/
/* object validation checks */
void kobject_test_user_13(void *p1, void *p2, void *p3)
{
	valid_fault = true;
	USERSPACE_BARRIER;

	k_sem_take(&kobject_sem_no_init_access, K_SECONDS(1));
	zassert_unreachable("_SYSCALL_OBJ implementation failure.");
}

/* Test syscall on a kobject which is not initialized and has access */
void test_access_kobject_without_init_with_access(void *p1,
						  void *p2, void *p3)
{
	k_thread_access_grant(k_current_get(),
			      &kobject_sem_no_init_access, NULL);

	k_thread_create(&kobject_test_13_tid,
			kobject_stack_1,
			KOBJECT_STACK_SIZE,
			kobject_test_user_13,
			NULL, NULL, NULL,
			0, K_INHERIT_PERMS | K_USER, K_NO_WAIT);


	k_sem_take(&sync_sem, SYNC_SEM_TIMEOUT);

}

/****************************************************************************/
/* object validation checks */
void kobject_test_user_2_14(void *p1, void *p2, void *p3)
{
	zassert_unreachable("_SYSCALL_OBJ implementation failure.");
}

void kobject_test_user_1_14(void *p1, void *p2, void *p3)
{
	valid_fault = true;
	USERSPACE_BARRIER;

	k_thread_create(&kobject_test_14_tid,
			kobject_stack_1,
			KOBJECT_STACK_SIZE,
			kobject_test_user_2_14,
			NULL, NULL, NULL,
			0, K_USER, K_NO_WAIT);

	zassert_unreachable("_SYSCALL_OBJ implementation failure.");

}
/* Try to reinitialize the k_thread object. Object validation check */
void test_kobject_reinitialize_thread_kobj(void *p1, void *p2, void *p3)
{
	k_thread_create(&kobject_test_14_tid,
			kobject_stack_1,
			KOBJECT_STACK_SIZE,
			kobject_test_user_1_14,
			NULL, NULL, NULL,
			0, K_INHERIT_PERMS | K_USER, K_NO_WAIT);


	k_sem_take(&sync_sem, SYNC_SEM_TIMEOUT);
}

/****************************************************************************/
/* object validation checks */
void kobject_test_user_2_15(void *p1, void *p2, void *p3)
{
	ztest_test_pass();
}

void kobject_test_user_1_15(void *p1, void *p2, void *p3)
{
	valid_fault = false;
	USERSPACE_BARRIER;

	k_thread_create(&kobject_test_reuse_4_tid,
			kobject_stack_2,
			KOBJECT_STACK_SIZE,
			kobject_test_user_2_15,
			NULL, NULL, NULL,
			0, K_USER, K_NO_WAIT);

	k_thread_abort(k_current_get());

}
/* Try to reintialize the k_thread object. Object validation check */
void test_create_new_thread_from_user(void *p1, void *p2, void *p3)
{

	k_thread_access_grant(&kobject_test_reuse_3_tid,
			      &kobject_test_reuse_4_tid,
			      &kobject_stack_2, NULL);

	k_thread_create(&kobject_test_reuse_3_tid,
			kobject_stack_1,
			KOBJECT_STACK_SIZE,
			kobject_test_user_1_15,
			NULL, NULL, NULL,
			0, K_INHERIT_PERMS | K_USER, K_NO_WAIT);


	k_sem_take(&sync_sem, SYNC_SEM_TIMEOUT);

}

/****************************************************************************/
/* object validation checks */
void kobject_test_user_2_16(void *p1, void *p2, void *p3)
{
	zassert_unreachable("k_object validation failure in k thread create");
}

void kobject_test_user_1_16(void *p1, void *p2, void *p3)
{
	valid_fault = true;
	USERSPACE_BARRIER;

	k_thread_create(&kobject_test_reuse_6_tid,
			kobject_stack_2,
			KOBJECT_STACK_SIZE,
			kobject_test_user_2_16,
			NULL, NULL, NULL,
			0, K_USER, K_NO_WAIT);

	k_thread_abort(k_current_get());

}
/* Create a new thread from user and the user doesn't have access
 * to the stack region of new thread.
 * _handler_k_thread_create validation.
 */
void test_create_new_thread_from_user_no_access_stack(void *p1,
						      void *p2, void *p3)
{

	k_thread_access_grant(&kobject_test_reuse_5_tid,
			      &kobject_test_reuse_6_tid, NULL);

	k_thread_create(&kobject_test_reuse_5_tid,
			kobject_stack_1,
			KOBJECT_STACK_SIZE,
			kobject_test_user_1_16,
			NULL, NULL, NULL,
			0, K_USER, K_NO_WAIT);


	k_sem_take(&sync_sem, SYNC_SEM_TIMEOUT);
}

/****************************************************************************/
/* object validation checks */
void kobject_test_user_2_17(void *p1, void *p2, void *p3)
{
	zassert_unreachable("k_object validation failure in k thread create");
}

void kobject_test_user_1_17(void *p1, void *p2, void *p3)
{

	valid_fault = true;
	USERSPACE_BARRIER;

	k_thread_create(&kobject_test_reuse_2_tid,
			kobject_stack_4,
			-1,
			kobject_test_user_2_17,
			NULL, NULL, NULL,
			0, K_USER, K_NO_WAIT);

	zassert_unreachable("k_object validation failure in k thread create");
}
/* Create a new thread from user and use a huge stack size which overflows
 * _handler_k_thread_create validation.
 */
#ifndef CONFIG_MPU_REQUIRES_POWER_OF_TWO_ALIGNMENT
void test_create_new_thread_from_user_invalid_stacksize(void *p1,
							void *p2, void *p3)
{

	k_thread_access_grant(&kobject_test_reuse_1_tid,
			      &kobject_test_reuse_2_tid,
			      &kobject_stack_3, NULL);

	k_thread_create(&kobject_test_reuse_1_tid,
			kobject_stack_3,
			KOBJECT_STACK_SIZE,
			kobject_test_user_1_17,
			NULL, NULL, NULL,
			0, K_USER, K_NO_WAIT);

	k_sem_take(&sync_sem, SYNC_SEM_TIMEOUT);

}
#else
void test_create_new_thread_from_user_invalid_stacksize(void *p1,
							void *p2, void *p3)
{
	ztest_test_skip();
}
#endif

/****************************************************************************/
/* object validation checks */
void kobject_test_user_2_18(void *p1, void *p2, void *p3)
{
	zassert_unreachable("k_object validation failure in k thread create");
}

void kobject_test_user_1_18(void *p1, void *p2, void *p3)
{

	valid_fault = true;
	USERSPACE_BARRIER;


	k_thread_create(&kobject_test_reuse_4_tid,
			kobject_stack_2,
			K_THREAD_STACK_SIZEOF(kobject_stack_2) + 1,
			kobject_test_user_2_18,
			NULL, NULL, NULL,
			0, K_USER, K_NO_WAIT);

	zassert_unreachable("k_object validation failure in k thread create");
}
/* Create a new thread from user and use a stack bigger than allowed size.
 * _handler_k_thread_create validation.
 */

#ifndef CONFIG_MPU_REQUIRES_POWER_OF_TWO_ALIGNMENT
void test_create_new_thread_from_user_huge_stacksize(void *p1,
						     void *p2, void *p3)
{

	k_thread_access_grant(&kobject_test_reuse_3_tid,
			      &kobject_test_reuse_4_tid,
			      &kobject_stack_2, NULL);

	k_thread_create(&kobject_test_reuse_3_tid,
			kobject_stack_1,
			KOBJECT_STACK_SIZE,
			kobject_test_user_1_18,
			NULL, NULL, NULL,
			0, K_USER, K_NO_WAIT);

	k_sem_take(&sync_sem, SYNC_SEM_TIMEOUT);

}
#else
void test_create_new_thread_from_user_huge_stacksize(void *p1,
						     void *p2, void *p3)
{
	ztest_test_skip();
}
#endif

/****************************************************************************/
/* object validation checks */

void kobject_test_user_2_19(void *p1, void *p2, void *p3)
{
	zassert_unreachable("k_object validation failure in k thread create");
}

void kobject_test_user_1_19(void *p1, void *p2, void *p3)
{

	valid_fault = true;
	USERSPACE_BARRIER;

	k_thread_create(&kobject_test_reuse_8_tid,
			kobject_stack_2,
			KOBJECT_STACK_SIZE,
			kobject_test_user_2_19,
			NULL, NULL, NULL,
			0, 0, K_NO_WAIT);

	zassert_unreachable("k_object validation failure in k thread create");
}
/* Create a new supervisor thread from user.
 * _handler_k_thread_create validation.
 */

void test_create_new_supervisor_thread_from_user(void *p1, void *p2, void *p3)
{
	k_thread_access_grant(&kobject_test_reuse_7_tid,
			      &kobject_test_reuse_8_tid,
			      &kobject_stack_2, NULL);

	k_thread_create(&kobject_test_reuse_7_tid,
			kobject_stack_1,
			KOBJECT_STACK_SIZE,
			kobject_test_user_1_19,
			NULL, NULL, NULL,
			0, K_USER, K_NO_WAIT);


	k_sem_take(&sync_sem, SYNC_SEM_TIMEOUT);

}

/****************************************************************************/
/* object validation checks */

void kobject_test_user_2_20(void *p1, void *p2, void *p3)
{
	zassert_unreachable("k_object validation failure in k thread create");
}

void kobject_test_user_1_20(void *p1, void *p2, void *p3)
{

	valid_fault = true;
	USERSPACE_BARRIER;

	k_thread_create(&kobject_test_reuse_2_tid,
			kobject_stack_2,
			KOBJECT_STACK_SIZE,
			kobject_test_user_2_20,
			NULL, NULL, NULL,
			0, K_USER | K_ESSENTIAL, K_NO_WAIT);

	zassert_unreachable("k_object validation failure in k thread create");
}
/* Create a new essential thread from user.
 * _handler_k_thread_create validation.
 */

void test_create_new_essential_thread_from_user(void *p1, void *p2, void *p3)
{
	k_thread_access_grant(&kobject_test_reuse_1_tid,
			      &kobject_test_reuse_2_tid,
			      &kobject_stack_2, NULL);

	k_thread_create(&kobject_test_reuse_1_tid,
			kobject_stack_1,
			KOBJECT_STACK_SIZE,
			kobject_test_user_1_20,
			NULL, NULL, NULL,
			0, K_USER, K_NO_WAIT);


	k_sem_take(&sync_sem, SYNC_SEM_TIMEOUT);

}

/****************************************************************************/
/* object validation checks */

void kobject_test_user_2_21(void *p1, void *p2, void *p3)
{
	zassert_unreachable("k_object validation failure in k thread create");
}

void kobject_test_user_1_21(void *p1, void *p2, void *p3)
{

	valid_fault = true;
	USERSPACE_BARRIER;

	k_thread_create(&kobject_test_reuse_4_tid,
			kobject_stack_2,
			KOBJECT_STACK_SIZE,
			kobject_test_user_2_21,
			NULL, NULL, NULL,
			-1, K_USER, K_NO_WAIT);

	zassert_unreachable("k_object validation failure in k thread create");
}
/* Create a new thread whose prority is higher than the current thread.
 * _handler_k_thread_create validation.
 */

void test_create_new_higher_prio_thread_from_user(void *p1, void *p2, void *p3)
{
	k_thread_access_grant(&kobject_test_reuse_3_tid,
			      &kobject_test_reuse_4_tid,
			      &kobject_stack_2, NULL);

	k_thread_create(&kobject_test_reuse_3_tid,
			kobject_stack_1,
			KOBJECT_STACK_SIZE,
			kobject_test_user_1_21,
			NULL, NULL, NULL,
			0, K_USER, K_NO_WAIT);


	k_sem_take(&sync_sem, SYNC_SEM_TIMEOUT);

}

/****************************************************************************/
/* object validation checks */

void kobject_test_user_2_22(void *p1, void *p2, void *p3)
{
	zassert_unreachable("k_object validation failure in k thread create");
}

void kobject_test_user_1_22(void *p1, void *p2, void *p3)
{

	valid_fault = true;
	USERSPACE_BARRIER;

	k_thread_create(&kobject_test_reuse_6_tid,
			kobject_stack_2,
			KOBJECT_STACK_SIZE,
			kobject_test_user_2_22,
			NULL, NULL, NULL,
			6000, K_USER, K_NO_WAIT);

	zassert_unreachable("k_object validation failure in k thread create");
}
/* Create a new thread whose prority is invalid.
 * _handler_k_thread_create validation.
 */

void test_create_new_invalid_prio_thread_from_user(void *p1, void *p2, void *p3)
{
	k_thread_access_grant(&kobject_test_reuse_5_tid,
			      &kobject_test_reuse_6_tid,
			      &kobject_stack_2, NULL);


	k_thread_create(&kobject_test_reuse_5_tid,
			kobject_stack_1,
			KOBJECT_STACK_SIZE,
			kobject_test_user_1_22,
			NULL, NULL, NULL,
			0, K_USER, K_NO_WAIT);

	k_sem_take(&sync_sem, SYNC_SEM_TIMEOUT);

}

