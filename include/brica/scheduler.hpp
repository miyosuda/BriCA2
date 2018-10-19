/******************************************************************************
 *
 * brica/scheduler.hpp
 *
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 *
 *****************************************************************************/

#ifndef __BRICA_KERNEL_SCHEDULER_HPP__
#define __BRICA_KERNEL_SCHEDULER_HPP__

#include "brica/component.hpp"

#include <functional>
#include <queue>
#include <vector>

#include <omp.h>

namespace brica {

using Time = unsigned long long;

struct Timing {
  Timing(Time offset, Time interval, Time sleep)
      : offset(offset), interval(interval), sleep(sleep) {}
  Time offset;
  Time interval;
  Time sleep;
};

static const std::function<void()> nop([]() {});

class VirtualTimePhasedScheduler {
 public:
  VirtualTimePhasedScheduler(std::function<void()> f = nop) : sync(f) {}

  void add_component(IComponent* component, std::size_t phase) {
    if (phase >= phases.size()) {
      phases.resize(phase + 1);
    }

    phases[phase].push_back(component);
  }

  void step() {
    for (std::size_t i = 0; i < phases.size(); ++i) {
#pragma omp parallel for
      for (std::size_t j = 0; j < phases[i].size(); ++j) {
        IComponent* component = phases[i][j];
        component->collect();
        component->execute();
      }
      sync();

#pragma omp parallel for
      for (std::size_t j = 0; j < phases[i].size(); ++j) {
        IComponent* component = phases[i][j];
        component->expose();
      }
      sync();
    }
  }

 private:
  std::vector<std::vector<IComponent*>> phases;
  std::function<void()> sync;
};

class VirtualTimeScheduler {
  struct Event {
    Time time;
    IComponent* component;
    Timing timing;
    bool sleep;

    bool operator<(const Event& rhs) const { return time > rhs.time; }
  };

 public:
  VirtualTimeScheduler(std::function<void()> f = nop) : sync(f) {}

  void add_component(IComponent* component, Timing timing) {
    event_queue.push({timing.offset, component, timing, false});
  }

  void step() {
    Time time = event_queue.top().time;

    std::queue<IComponent*> awake;
    std::queue<IComponent*> asleep;

    while (event_queue.top().time == time) {
      Event event = event_queue.top();
      event_queue.pop();

      Event next = event;

      if (event.sleep) {
        asleep.push(event.component);
        next.time += next.timing.sleep;
        next.sleep = false;
      } else {
        awake.push(event.component);
        next.time += next.timing.interval;
        next.sleep = true;
      }

      event_queue.push(next);
    }

#pragma omp parallel
    while (!asleep.empty()) {
      IComponent* component = asleep.front();
      asleep.pop();
      component->expose();
    }
    sync();

#pragma omp parallel
    while (!awake.empty()) {
      IComponent* component = awake.front();
      awake.pop();
      component->collect();
      component->execute();
    }
    sync();
  }

 private:
  std::priority_queue<Event> event_queue;
  std::function<void()> sync;
};

}  // namespace brica

#endif  // __BRICA_KERNEL_SCHEDULER_HPP__
