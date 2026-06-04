# Tessera — Учебная программа

Физический движок на ECS + DOD + метапрограммирование.

Прогресс отмечается `[x]`. Не лезть в фазу N+1 без закрытия фазы N.

---

## Текущий фокус

**Фаза 1, пункты 1–3** (линейная алгебра, матрицы, кватернионы).

---

## Фаза 1 — Математика и физика (фундамент)

- [ ] 1. Линейная алгебра: вектор, скалярное произв, векторное произв, длина, нормализация, базис.
- [ ] 2. Матрицы: умножение, transpose, inverse; матрица поворота 3x3.
- [ ] 3. Кватернионы: зачем нужны (gimbal lock), умножение, нормализация, conversion с матрицей.
- [ ] 4. Ньютоновская механика: F=ma, импульс, момент, момент инерции (tensor для 3D), кинетическая энергия.
- [ ] 5. Вращения тел: угловая скорость, угловое ускорение, момент силы (torque).
- [ ] 6. Численное интегрирование: Euler explicit / implicit / semi-implicit, Verlet, RK4. Стабильность vs точность.

## Фаза 2 — Компьютерная архитектура (для DOD)

- [ ] 7. Memory hierarchy: L1/L2/L3, cache line, latency-цифры.
- [ ] 8. Cache miss / prefetcher / locality: spatial vs temporal.
- [ ] 9. TLB и страничная память.
- [ ] 10. SoA vs AoS на бенчмарках.
- [ ] 11. Branch prediction, branchless code.
- [ ] 12. SIMD: SSE/AVX intrinsics, alignment 16/32, packed vs scalar.
- [ ] 13. Allocators: pool, arena, free-list, bump.

## Фаза 3 — Современный C++ для метапрограммирования

- [ ] 14. Шаблоны: function/class, partial/full specialization.
- [ ] 15. Variadic templates + fold expressions (C++17).
- [ ] 16. Type traits.
- [ ] 17. SFINAE (для чтения legacy).
- [ ] 18. C++20 concepts & requires.
- [ ] 19. `if constexpr`.
- [ ] 20. `constexpr` / `consteval`.
- [ ] 21. Type lists: TypeList<T...>, операции.
- [ ] 22. Tag dispatching и CRTP.

## Фаза 4 — ECS внутренности

- [ ] 23. Entity как handle (index + generation), versioning, recycle.
- [ ] 24. Sparse set.
- [ ] 25. Archetype storage, чанки, миграция.
- [ ] 26. Component registry: compile-time vs runtime ID.
- [ ] 27. Query / iteration: `ForEach<C1, C2, ...>` без overhead.
- [ ] 28. System scheduling: dependency graph, parallelism.

## Фаза 5 — Физика алгоритмически

- [ ] 29. Broad-phase: spatial hash, sweep-and-prune, AABB tree, dynamic BVH.
- [ ] 30. Narrow-phase для convex: SAT, GJK, EPA.
- [ ] 31. Narrow-phase для примитивов: sphere-sphere, sphere-plane, sphere-box, box-box.
- [ ] 32. Contact manifold: persistence между кадрами.
- [ ] 33. Constraint solver: impulse-based, Sequential Impulses, PGS, projection.
- [ ] 34. Friction model: Coulomb friction, friction cone.
- [ ] 35. Restitution и stacking.
- [ ] 36. Sleeping / island management.
- [ ] 37. CCD: TOI, bullets.

## Фаза 6 — Интеграция и продакшн темы

- [ ] 38. Determinism: что ломает, как сохранить.
- [ ] 39. Multithreading: job system, parallel-for, false sharing.
- [ ] 40. API design: opaque handles, ABI, pImpl, версионирование.
- [ ] 41. Сериализация: snapshot, delta, compression.
- [ ] 42. Profiling: tracy, vtune, perf.
- [ ] 43. Тестирование физики: детерминизм, regression-сцены, fuzzing.

---

## Источники

- **Фаза 1:** *3D Math Primer for Graphics and Game Development* (Dunn & Parberry)
- **Фаза 2:** *Computer Systems: A Programmer's Perspective* (Bryant & O'Hallaron) + Mike Acton CppCon 2014
- **Фаза 3:** *C++ Templates: The Complete Guide* (Vandevoorde et al., 2nd ed.)
- **Фаза 4:** Sander Mertens ECS FAQ + EnTT docs + Bevy ECS guide
- **Фаза 5:** *Real-Time Collision Detection* (Christer Ericson) + Erin Catto GDC slides + *Game Physics Engine Development* (Ian Millington)
- **Фаза 6:** Jolt Physics source + gafferongames.com (Glenn Fiedler)

---

## Лог прогресса

<!-- Записи "что сделано, что нет" — сюда. Дата + краткий итог. -->
