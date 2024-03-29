/* stats.c                  -*-C-*-
 *
 *************************************************************************
 *
 *  Copyright (C) 2009-2016, Intel Corporation
 *  All rights reserved.
 *  
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *  
 *    * Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *    * Redistributions in binary form must reproduce the above copyright
 *      notice, this list of conditions and the following disclaimer in
 *      the documentation and/or other materials provided with the
 *      distribution.
 *    * Neither the name of Intel Corporation nor the names of its
 *      contributors may be used to endorse or promote products derived
 *      from this software without specific prior written permission.
 *  
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *  HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 *  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 *  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 *  OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 *  AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 *  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY
 *  WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 *  
 *  *********************************************************************
 *  
 *  PLEASE NOTE: This file is a downstream copy of a file mainitained in
 *  a repository at cilkplus.org. Changes made to this file that are not
 *  submitted through the contribution process detailed at
 *  http://www.cilkplus.org/submit-cilk-contribution will be lost the next
 *  time that a new version is released. Changes only submitted to the
 *  GNU compiler collection or posted to the git repository at
 *  https://bitbucket.org/intelcilkruntime/itnel-cilk-runtime.git are
 *  not tracked.
 *  
 *  We welcome your contributions to this open source project. Thank you
 *  for your assistance in helping us improve Cilk Plus.
 **************************************************************************/

#include "stats.h"
#include "bug.h"
#include "os.h"
#include "local_state.h"

#include <stdio.h>

#define INVALID_START (0ULL - 1ULL)

#ifdef CILK_PROFILE
/* MSVC does not support designated initializers, grrrr... */
static const char *names[] = {
    /*[INTERVAL_IN_SCHEDULER]*/                 "in scheduler",
    /*[INTERVAL_SEARCHING]*/                    "  of which: searching", // ARTHUR
    /*[INTERVAL_WORKING]*/                      "  of which: working",
    /*[INTERVAL_IN_RUNTIME]*/                   "  of which: in runtime",
    /*[INTERVAL_IN_SCHED_LOOP]*/                "     of which: in sched loop",
    /*[INTERVAL_STEALING]*/                     "        of which: stealing",
    /*[INTERVAL_STEAL_SUCCESS]*/                "steal success: detach",
    /*[INTERVAL_STEAL_FAIL_EMPTYQ]*/            "steal fail: empty queue",
    /*[INTERVAL_STEAL_FAIL_LOCK]*/              "steal fail: victim locked",
    /*[INTERVAL_STEAL_FAIL_USER_WORKER]*/       "steal fail: user worker",
    /*[INTERVAL_STEAL_FAIL_DEKKER]*/            "steal fail: dekker",
    /*[INTERVAL_SYNC_CHECK]*/                   "sync check",
    /*[INTERVAL_THE_EXCEPTION_CHECK]*/          "THE exception check",
    /*[INTERVAL_THE_EXCEPTION_CHECK_USELESS]*/  "  of which: useless",
    /*[INTERVAL_RETURNING]*/                    "returning",
    /*[INTERVAL_FINALIZE_CHILD]*/               "finalize child",
    /*[INTERVAL_PROVABLY_GOOD_STEAL]*/          "provably good steal",
    /*[INTERVAL_UNCONDITIONAL_STEAL]*/          "unconditional steal",
    /*[INTERVAL_ALLOC_FULL_FRAME]*/             "alloc full frame",
    /*[INTERVAL_FRAME_ALLOC_LARGE]*/            "large frame alloc",
    /*[INTERVAL_FRAME_ALLOC]*/                  "small frame alloc",
    /*[INTERVAL_FRAME_ALLOC_GLOBAL]*/           "  of which: to global pool",
    /*[INTERVAL_FRAME_FREE_LARGE]*/             "large frame free",
    /*[INTERVAL_FRAME_FREE]*/                   "small frame free",
    /*[INTERVAL_FRAME_FREE_GLOBAL]*/            "  of which: to global pool",
    /*[INTERVAL_MUTEX_LOCK]*/                   "mutex lock",
    /*[INTERVAL_MUTEX_LOCK_SPINNING]*/          "  spinning",
    /*[INTERVAL_MUTEX_LOCK_YIELDING]*/          "  yielding",
    /*[INTERVAL_MUTEX_TRYLOCK]*/                "mutex trylock",
    /*[INTERVAL_FIBER_ALLOCATE]*/               "fiber_allocate",
    /*[INTERVAL_FIBER_DEALLOCATE]*/             "fiber_deallocate", 
    /*[INTERVAL_FIBER_ALLOCATE_FROM_THREAD]*/   "fiber_allocate_from_thread",
    /*[INTERVAL_FIBER_DEALLOCATE_FROM_THREAD]*/ "fiber_deallocate (thread)", 
    /*[INTERVAL_SUSPEND_RESUME_OTHER]*/         "fiber suspend self + resume",
    /*[INTERVAL_DEALLOCATE_RESUME_OTHER]*/      "fiber deallocate self + resume", 
    /*[INTERVAL_INIT_WORKER]*/                  "init worker thread", 
    /*[INTERVAL_SCHEDULE_WAIT]*/                "schedule wait state", 
};
#endif

void __cilkrts_init_stats(statistics *s)
{
    int i;
    for (i = 0; i < INTERVAL_N; ++i) {
        s->start[i] = INVALID_START;
        s->accum[i] = 0;
        s->count[i] = 0;
    }

    s->stack_hwm = 0;
}

unsigned long long _nanoseconds_of(unsigned long long cpu_frequency_khz, unsigned long long cycles) {
  return 1000000l * cycles / cpu_frequency_khz;
}

double _seconds_of_nanoseconds(unsigned long long ns) {
  return (double)ns / 1.0e9;
}

double _seconds_of_cycles(unsigned long long cpu_frequency_khz, unsigned long long cycles) {
  return _seconds_of_nanoseconds(_nanoseconds_of(cpu_frequency_khz, cycles));
}

void __cilkrts_dump_json_stats(FILE* f, unsigned long long cpu_frequency_khz, statistics *s)
{
    // DOCUMENTATION:
    // searching <= working 
    // working + runtime = total
    // total = exectime * nbcores * 2.4GHz
    // utilization = 1 - (searching / total)
    // printf("CILK ENCORE STATISTICS FROM SNAPSHOT UNTIL NOW:\n\n");
    double utilization = 1.0 - ((double) s->accum[INTERVAL_SEARCHING]) / ((double) s->accum[INTERVAL_IN_SCHEDULER]);
    fprintf(f, "{\"utilization\": %.5lf,\n", utilization);
    fprintf(f, "\"nb_steals\": %lld,\n", s->count[INTERVAL_STEAL_SUCCESS]);
    fprintf(f, "\"nb_fibers\": %lld,\n", s->count[INTERVAL_FIBER_ALLOCATE]);
    fprintf(f, "\"total_time\": %.5lf,\n",
	    _seconds_of_cycles(cpu_frequency_khz, s->accum[INTERVAL_IN_SCHEDULER]));
    fprintf(f, "\"total_work_time\": %.5lf,\n",
	    _seconds_of_cycles(cpu_frequency_khz, s->accum[INTERVAL_WORKING]));
    fprintf(f, "\"total_in_runtime\": %.5lf,\n",
	    _seconds_of_cycles(cpu_frequency_khz, s->accum[INTERVAL_IN_RUNTIME]));
    fprintf(f, "\"total_idle_time\": %.5lf}\n",
	    _seconds_of_cycles(cpu_frequency_khz, s->accum[INTERVAL_SEARCHING]));
}
//#endif

#ifdef CILK_PROFILE

void __cilkrts_accum_stats(statistics *to, statistics *from)
{
    int i;

    //#ifdef ARTHUR
    unsigned long long now = __cilkrts_getticks();
    //#endif

    for (i = 0; i < INTERVAL_N; ++i) {
        to->accum[i] += from->accum[i];
        to->count[i] += from->count[i];
        from->accum[i] = 0;
        from->count[i] = 0;
        //#ifdef ARTHUR
        // If an interval is not closed, we count its duration until now
        if (from->start[i] != INVALID_START) {
           to->accum[i] += now - from->start[i];
           from->start[i] = now;
        }
        //#endif
    }

    if (from->stack_hwm > to->stack_hwm)
        to->stack_hwm = from->stack_hwm;
    from->stack_hwm = 0;
}

//#ifdef ARTHUR
void __cilkrts_nondestructive_accum_stats(statistics *to, statistics *from, int multiplier)
{
    unsigned long long now = __cilkrts_getticks();
    int i;
    for (i = 0; i < INTERVAL_N; ++i) {
        to->accum[i] += from->accum[i] * multiplier;
        to->count[i] += from->count[i] * multiplier;
        // special case for INTERVAL_IN_SCHEDULER: we should not zero it out
        if (i == INTERVAL_IN_SCHEDULER && to->count[i] == 0 && from->count[i] != 0) {
          to->count[i] = 1;
        }
        // If an interval is not closed, we count its duration until now
        if (from->start[i] != INVALID_START) {
           to->accum[i] += (now - from->start[i]) * multiplier;
        }
    }

    if (from->stack_hwm > to->stack_hwm)
        to->stack_hwm = from->stack_hwm;
}
//#endif

void __cilkrts_note_interval(__cilkrts_worker *w, enum interval i)
{
    if (w) {
        statistics *s = w->l->stats;
        CILK_ASSERT(s->start[i] == INVALID_START);
        s->count[i]++;
    }
}

void __cilkrts_start_interval(__cilkrts_worker *w, enum interval i)
{
    if (w) {
        statistics *s = w->l->stats;
        CILK_ASSERT(s->start[i] == INVALID_START);
        s->start[i] = __cilkrts_getticks();
        s->count[i]++;
    }
}

void __cilkrts_stop_interval(__cilkrts_worker *w, enum interval i)
{
    if (w) {
        statistics *s = w->l->stats;
        CILK_ASSERT(s->start[i] != INVALID_START);
        s->accum[i] += __cilkrts_getticks() - s->start[i];
        s->start[i] = INVALID_START;
    }
}

void dump_stats_to_file(FILE *stat_file, statistics *s)
{
    // Only print out stats for worker if they are nonzero.
    // #ifdef ARTHUR was: // if (s->accum[INTERVAL_IN_SCHEDULER] > 0) {
    if (s->count[INTERVAL_IN_SCHEDULER] > 0) {
        int i;
        fprintf(stat_file, "\nCILK PLUS RUNTIME SYSTEM STATISTICS:\n\n");
        fprintf(stat_file,
                "  %-32s: %15s %10s %12s %10s\n",
                "event",
                "count",
                "ticks",
                "ticks/count",
                "%total"
            );
        for (i = 0; i < INTERVAL_N; ++i) {
            fprintf(stat_file, "  %-32s: %15llu", names[i], s->count[i]);
            if (s->accum[i]) {
                fprintf(stat_file, " %10.3g %12.3g %10.2f",
                        (double)s->accum[i],
                        (double)s->accum[i] / (double)s->count[i],
                        100.0 * (double)s->accum[i] / 
                        (double)s->accum[INTERVAL_IN_SCHEDULER]);
            }
            fprintf(stat_file, "\n");
        }
    }
    else {
        fprintf(stat_file, "empty statistics\n");
    }
}
#endif // CILK_PROFILE

/* End stats.c */
