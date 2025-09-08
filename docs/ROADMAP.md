## Fleur Engine – Refactoring & Evolution Road-map

> A living document that captures the **why**, **what**, and **order** of upcoming changes.  Keep it under version control and adjust as we learn.

> Custom Allocator: 
The first step is to implement a custom allocator. Memory management is the foundation for all higher-level systems.

> Service Locator refactoring: After the allocator, the next priority is introducing a service locator to replace ad-hoc singletons. The locator will manage global services like logging, windowing, and rendering, allocating them through the custom allocator to avoid static initialization order issues. This creates a stable backbone where services can be initialized, accessed, and shut down in a controlled way.

> ECS: Once service management is in place, reflection becomes important for automation and tooling. At this stage, we can add a simple registration system for services and components, allowing discovery by name. Reflection then enables integration with the editor (e.g. listing services in a debug UI) and serialization (e.g. JSON or YAML), preparing the ground for scripting and component inspection.

With these foundations ready, the engine can move toward an Entity-Component-System. ECS will provide a flexible way to store and process game data, leveraging the allocator for efficient component pools and optionally using reflection for automatic registration and serialization. A minimal ECS would include entity IDs, sparse or dense component storage, and iteration over sets of components. On top of that, systems can drive game logic, while later integration with scripting and the editor will make ECS more dynamic.

The overall order is: custom allocator → service locator → reflection → ECS. This sequence ensures each new layer builds on stable lower-level abstractions, avoiding rework and enabling incremental progress. At every step, the engine should remain in a working state, gradually evolving into a more powerful and flexible architecture.
