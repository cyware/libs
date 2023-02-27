/*
Copyright (C) 2023 The Falco Authors.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

*/

#include <gtest/gtest.h>
#include <sinsp.h>
#include <sys/syscall.h>
#include "../test_utils.h"
// We need to include syscall compat tables
#ifdef __x86_64__
#include "syscall_compat_x86_64.h"
#elif __aarch64__
#include "syscall_compat_aarch64.h"
#elif __s390x__
#include "syscall_compat_s390x.h"
#endif /* __x86_64__ */

TEST(events_set, check_size)
{
	auto sc_set = libsinsp::events::set<ppm_sc_code>();
	ASSERT_EQ(sc_set.size(), 0);
	ASSERT_TRUE(sc_set.empty());

	sc_set.insert(PPM_SC_ACCEPT);
	ASSERT_EQ(sc_set.size(), 1);

	sc_set.insert(PPM_SC_ACCEPT);
	ASSERT_EQ(sc_set.size(), 1);

	sc_set.remove(PPM_SC_ACCEPT4);
	ASSERT_EQ(sc_set.size(), 1);

	sc_set.insert(PPM_SC_ACCEPT4);
	ASSERT_EQ(sc_set.size(), 2);

	sc_set.clear();
	ASSERT_EQ(sc_set.size(), 0);
	ASSERT_TRUE(sc_set.empty());
}

TEST(events_set, check_equal)
{
	auto sc_set = libsinsp::events::set<ppm_sc_code>();
	sc_set.insert(PPM_SC_ACCEPT);
	sc_set.insert(PPM_SC_ACCEPT4);

	auto other_set = libsinsp::events::set<ppm_sc_code>();
	ASSERT_FALSE(sc_set.equals(other_set));

	other_set.insert(PPM_SC_ACCEPT);
	ASSERT_FALSE(sc_set.equals(other_set));

	other_set.insert(PPM_SC_ACCEPT4);
	ASSERT_TRUE(sc_set.equals(other_set));

	sc_set.clear();
	ASSERT_FALSE(sc_set.equals(other_set));
	other_set.clear();
	ASSERT_TRUE(sc_set.equals(other_set));
	ASSERT_TRUE(sc_set.equals(libsinsp::events::set<ppm_sc_code>()));
	ASSERT_TRUE(other_set.equals(libsinsp::events::set<ppm_sc_code>()));
}

TEST(events_set, set_check_merge)
{
	auto merge_vec = std::vector<uint8_t>{1,2,3,4,5};
	auto intersect_vector = std::vector<uint8_t>{1,2,3,4,5};
	auto difference_vector = std::vector<uint8_t>{1,2,3,4,5};

	auto sc_set_1 = libsinsp::events::set<ppm_sc_code>();
	sc_set_1.insert((ppm_sc_code)1);
	sc_set_1.insert((ppm_sc_code)4);

	auto sc_set_2 = libsinsp::events::set<ppm_sc_code>();
	sc_set_2.insert((ppm_sc_code)1);
	sc_set_2.insert((ppm_sc_code)2);
	sc_set_2.insert((ppm_sc_code)3);
	sc_set_2.insert((ppm_sc_code)5);

	auto sc_set_merge = sc_set_1.merge(sc_set_2);
	for (auto val : merge_vec) {
		ASSERT_EQ(sc_set_merge.data()[val], 1);
	}
}

TEST(events_set, set_check_intersect)
{
	auto int_vec = std::vector<uint8_t>{1,4};

	auto sc_set_1 = libsinsp::events::set<ppm_sc_code>();
	sc_set_1.insert((ppm_sc_code)1);
	sc_set_1.insert((ppm_sc_code)4);

	auto sc_set_2 = libsinsp::events::set<ppm_sc_code>();
	sc_set_2.insert((ppm_sc_code)1);
	sc_set_2.insert((ppm_sc_code)2);
	sc_set_2.insert((ppm_sc_code)4);
	sc_set_2.insert((ppm_sc_code)5);

	auto sc_set_int = sc_set_1.intersect(sc_set_2);
	for (auto val : int_vec) {
		ASSERT_EQ(sc_set_int.data()[val], 1);
	}
}

TEST(events_set, set_check_diff)
{
	auto diff_vec = std::vector<uint8_t>{2,3};

	auto sc_set_1 = libsinsp::events::set<ppm_sc_code>();
	sc_set_1.insert((ppm_sc_code)1);
	sc_set_1.insert((ppm_sc_code)2);
	sc_set_1.insert((ppm_sc_code)3);
	sc_set_1.insert((ppm_sc_code)4);

	auto sc_set_2 = libsinsp::events::set<ppm_sc_code>();
	sc_set_2.insert((ppm_sc_code)1);
	sc_set_2.insert((ppm_sc_code)4);

	auto sc_set_diff = sc_set_1.diff(sc_set_2);
	for (auto val : diff_vec) {
		ASSERT_TRUE(sc_set_diff.contains((ppm_sc_code)val));
	}
}

TEST(events_set, names_to_event_set)
{
	auto event_set = libsinsp::events::names_to_event_set(std::unordered_set<std::string>{"openat","execveat"});
	libsinsp::events::set<ppm_event_code> event_set_truth = {PPME_SYSCALL_OPENAT_E, PPME_SYSCALL_OPENAT_X,
	PPME_SYSCALL_OPENAT_2_E, PPME_SYSCALL_OPENAT_2_X, PPME_SYSCALL_EXECVEAT_E, PPME_SYSCALL_EXECVEAT_X};
	ASSERT_PPM_EVENT_CODES_EQ(event_set_truth, event_set);
	ASSERT_EQ(event_set.size(), 6); // enter/exit events for each event name, special case "openat" has 4 PPME instead of 2

	// generic event case
	event_set = libsinsp::events::names_to_event_set(std::unordered_set<std::string>{"openat","execveat","syncfs"});
	event_set_truth = {PPME_SYSCALL_OPENAT_E, PPME_SYSCALL_OPENAT_X, PPME_SYSCALL_OPENAT_2_E, PPME_SYSCALL_OPENAT_2_X,
	PPME_SYSCALL_EXECVEAT_E, PPME_SYSCALL_EXECVEAT_X, PPME_GENERIC_E, PPME_GENERIC_X};
	ASSERT_PPM_EVENT_CODES_EQ(event_set_truth, event_set);
	ASSERT_EQ(event_set.size(), 8); // enter/exit events for each event name, special case "openat" has 4 PPME instead of 2
}

TEST(events_set, event_set_to_names)
{
	static std::set<std::string> names_truth = {"kill", "dup", "umount", "umount2", "eventfd", "eventfd2", "procexit", "container"};
	auto names = test_utils::unordered_set_to_ordered(libsinsp::events::event_set_to_names(libsinsp::events::set<ppm_event_code>{PPME_SYSCALL_KILL_E, PPME_SYSCALL_KILL_X,
	PPME_SYSCALL_DUP_1_E, PPME_SYSCALL_DUP_1_X, PPME_SYSCALL_UMOUNT_E, PPME_SYSCALL_UMOUNT_X, PPME_SYSCALL_EVENTFD_E, PPME_SYSCALL_EVENTFD_X, PPME_PROCEXIT_E, PPME_CONTAINER_E, PPME_CONTAINER_X}));
	ASSERT_NAMES_EQ(names_truth, names);
}

TEST(events_set, event_set_to_names_generic_events)
{
	static libsinsp::events::set<ppm_event_code> generic_event_set = {PPME_GENERIC_E, PPME_GENERIC_X};
	auto names = libsinsp::events::event_set_to_names(generic_event_set);
	/* Negative assertions. */
	ASSERT_TRUE(unordered_set_intersection(names, std::unordered_set<std::string> {"execve"}).empty());
	ASSERT_TRUE(unordered_set_intersection(names, std::unordered_set<std::string> {"accept"}).empty());
	ASSERT_TRUE(unordered_set_intersection(names, std::unordered_set<std::string> {"mprotect"}).empty());
	ASSERT_TRUE(unordered_set_intersection(names, std::unordered_set<std::string> {"mmap"}).empty());
	ASSERT_TRUE(unordered_set_intersection(names, std::unordered_set<std::string> {"container"}).empty());
	ASSERT_TRUE(unordered_set_intersection(names, std::unordered_set<std::string> {"procexit"}).empty());
	/* Random checks for some generic sc events. */
	ASSERT_FALSE(unordered_set_intersection(names, std::unordered_set<std::string> {"syncfs"}).empty());
	ASSERT_FALSE(unordered_set_intersection(names, std::unordered_set<std::string> {"perf_event_open"}).empty());
	ASSERT_FALSE(unordered_set_intersection(names, std::unordered_set<std::string> {"timer_create"}).empty());
	ASSERT_FALSE(unordered_set_intersection(names, std::unordered_set<std::string> {"lsetxattr"}).empty());
	ASSERT_FALSE(unordered_set_intersection(names, std::unordered_set<std::string> {"getsid"}).empty());
	ASSERT_FALSE(unordered_set_intersection(names, std::unordered_set<std::string> {"init_module"}).empty());
	ASSERT_FALSE(unordered_set_intersection(names, std::unordered_set<std::string> {"sethostname"}).empty());
	ASSERT_FALSE(unordered_set_intersection(names, std::unordered_set<std::string> {"readlinkat"}).empty());

	/* Solely check for some conservative lower bound to roughly ensure
	 * we are getting a whole bunch of generic sc events.
	 * At the time of writing we have about 234 generic sc syscalls as defined
	 * by not having a dedicated PPME_SYSCALL_* or PPME_SOCKET_* definition.
	*/
	ASSERT_GT(names.size(), 180);
}

TEST(events_set, event_set_to_names_no_generic_events)
{
	auto names = libsinsp::events::event_set_to_names(libsinsp::events::all_event_set(), false);
	ASSERT_FALSE(unordered_set_intersection(names, std::unordered_set<std::string> {"execve"}).empty());
	ASSERT_FALSE(unordered_set_intersection(names, std::unordered_set<std::string> {"accept"}).empty());
	ASSERT_FALSE(unordered_set_intersection(names, std::unordered_set<std::string> {"mprotect"}).empty());
	ASSERT_FALSE(unordered_set_intersection(names, std::unordered_set<std::string> {"mmap"}).empty());
	ASSERT_FALSE(unordered_set_intersection(names, std::unordered_set<std::string> {"container"}).empty());
	ASSERT_FALSE(unordered_set_intersection(names, std::unordered_set<std::string> {"procexit"}).empty());

	ASSERT_TRUE(unordered_set_intersection(names, std::unordered_set<std::string> {"syncfs"}).empty());
	ASSERT_TRUE(unordered_set_intersection(names, std::unordered_set<std::string> {"perf_event_open"}).empty());
	ASSERT_TRUE(unordered_set_intersection(names, std::unordered_set<std::string> {"timer_create"}).empty());
	ASSERT_TRUE(unordered_set_intersection(names, std::unordered_set<std::string> {"lsetxattr"}).empty());
	ASSERT_TRUE(unordered_set_intersection(names, std::unordered_set<std::string> {"getsid"}).empty());
	ASSERT_TRUE(unordered_set_intersection(names, std::unordered_set<std::string> {"init_module"}).empty());
	ASSERT_TRUE(unordered_set_intersection(names, std::unordered_set<std::string> {"sethostname"}).empty());
	ASSERT_TRUE(unordered_set_intersection(names, std::unordered_set<std::string> {"readlinkat"}).empty());
}

TEST(events_set, sc_set_to_event_set)
{
	libsinsp::events::set<ppm_sc_code> sc_set = {
#ifdef __NR_kill
	PPM_SC_KILL,
#endif

#ifdef __NR_sendto
	PPM_SC_SENDTO,
#endif

#ifdef __NR_setresuid
	PPM_SC_SETRESUID, // note: corner case PPM_SC_SETRESUID32 would fail
#endif

#ifdef __NR_alarm
	PPM_SC_ALARM,
#endif
	};

	libsinsp::events::set<ppm_event_code> event_set_truth = {
#ifdef __NR_kill
	PPME_SYSCALL_KILL_E,
	PPME_SYSCALL_KILL_X,
#endif

#ifdef __NR_sendto
	PPME_SOCKET_SENDTO_E,
	PPME_SOCKET_SENDTO_X,
#endif

#ifdef __NR_setresuid
	PPME_SYSCALL_SETRESUID_E,
	PPME_SYSCALL_SETRESUID_X,
#endif

#ifdef __NR_alarm
	PPME_GENERIC_E,
	PPME_GENERIC_X,
#endif
	};

	auto event_set = libsinsp::events::sc_set_to_event_set(sc_set);
	ASSERT_PPM_EVENT_CODES_EQ(event_set_truth, event_set);
}

TEST(events_set, all_non_generic_sc_event_set)
{
	auto event_set = libsinsp::events::all_event_set().filter([&](ppm_event_code e) { return libsinsp::events::is_syscall_event(e); })\
	.diff(libsinsp::events::set<ppm_event_code>{PPME_GENERIC_E, PPME_GENERIC_X});
	/* No generic sc events expected. */
	ASSERT_FALSE(event_set.contains(PPME_GENERIC_E));
	ASSERT_FALSE(event_set.contains(PPME_GENERIC_X));
	/* No non sc events expected. */
	ASSERT_FALSE(event_set.contains(PPME_CONTAINER_E));
	ASSERT_FALSE(event_set.contains(PPME_CONTAINER_X));
	ASSERT_FALSE(event_set.contains(PPME_PROCEXIT_E));
	ASSERT_FALSE(event_set.contains(PPME_PROCEXIT_X));
}

TEST(events_set, all_non_sc_event_set)
{
	auto event_set = libsinsp::events::all_event_set().filter([&](ppm_event_code e) { return !libsinsp::events::is_syscall_event(e); });
	/* No sc events at all expected. */
	ASSERT_FALSE(event_set.contains(PPME_GENERIC_E));
	ASSERT_FALSE(event_set.contains(PPME_GENERIC_X));
	ASSERT_FALSE(event_set.contains(PPME_SOCKET_ACCEPT_E));
	ASSERT_FALSE(event_set.contains(PPME_SOCKET_ACCEPT_X));
	ASSERT_FALSE(event_set.contains(PPME_SYSCALL_OPENAT2_E));
	ASSERT_FALSE(event_set.contains(PPME_SYSCALL_OPENAT2_X));
	/* Some critical expected non sc events. */
	ASSERT_TRUE(event_set.contains(PPME_CONTAINER_E));
	ASSERT_TRUE(event_set.contains(PPME_CONTAINER_X));
	ASSERT_TRUE(event_set.contains(PPME_PROCEXIT_E));
	ASSERT_TRUE(event_set.contains(PPME_PROCEXIT_X));
}

// TODO -> future PR after other PRs have been merged and few other things clarified
TEST(events_set, sinsp_state_event_set)
{
}
