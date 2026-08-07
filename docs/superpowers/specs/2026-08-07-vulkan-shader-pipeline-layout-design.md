# Vulkan Shader and Pipeline Layout Design

## Goal

Разделить текущий `FVkShader`, который одновременно управляет shader modules, reflection и pipeline cache, на небольшие классы с одной ответственностью.

Первый этап касается только shader modules, reflection, descriptor set layouts и pipeline layout. Frequency classification и управление конкретными descriptor sets в этот этап не входят.

## Current problem

`FVkShader` сейчас:

- создаёт и уничтожает `VkShaderModule`;
- выполняет SPIR-V reflection;
- собирает vertex input;
- собирает descriptor bindings и push constants;
- создаёт и кэширует `FVkPipeline`.

Из-за этого shader abstraction знает о pipeline state и pipeline cache, а pipeline layout создаётся неявно во время создания pipeline.

## Target responsibilities

### `FVkShader`

Отвечает только за shader modules и результат reflection.

Хранит и предоставляет:

- vertex, geometry и fragment `VkShaderModule`;
- shader stages и entry points;
- descriptor bindings, сгруппированные по `set`;
- push constant ranges;
- vertex input description.

`FVkShader` не должен:

- создавать `VkDescriptorSetLayout`;
- создавать `VkPipelineLayout`;
- создавать или кэшировать `FVkPipeline`;
- назначать ресурсам новые `set` или `binding`.

`FVkShader` is non-copyable and non-movable. If module creation or reflection fails, already-created shader modules are released before the error is propagated.

Reflection из SPIR-V является источником истины для технических `set`, `binding`, descriptor type и descriptor count.

### `FVkPipelineLayout`

Отвечает за Vulkan layout, общий для pipeline и descriptor binding.

Он должен:

1. Получить reflection от `FVkShader`.
2. Объединить bindings всех shader stages по паре `(set, binding)`.
3. Объединить `stageFlags`, если один binding используется несколькими stages.
4. Проверить совпадение descriptor type и descriptor count.
5. Создать отдельный `VkDescriptorSetLayout` для каждого номера `set`.
6. Создать пустые layouts для отсутствующих промежуточных set'ов.
7. Создать общий `VkPipelineLayout` из массива set layouts и push constants.
8. Владеть и уничтожать созданные Vulkan handles.

Пример:

```text
shader reflection:
    set 0, binding 0 -> Camera UBO       [vertex, fragment]
    set 1, binding 0 -> Albedo texture   [fragment]
    set 2, binding 0 -> Transform UBO    [vertex]

pipeline layout:
    set layout 0 -> Camera UBO
    set layout 1 -> Albedo texture
    set layout 2 -> Transform UBO
```

Если два shader stage используют один `(set, binding)`, но указывают разные descriptor types или counts, создание layout должно завершиться диагностируемой ошибкой.

### `FVkPipeline`

Отвечает за graphics pipeline:

- shader stages;
- vertex input;
- topology;
- rasterization и culling;
- depth/stencil state;
- blending;
- attachment formats;
- `VkPipeline`.

`FVkPipeline` получает готовый `FVkPipelineLayout`. Он не должен сам собирать descriptor set layouts из shader reflection и не должен принимать независимый массив layouts, который может расходиться с pipeline layout.

### `FVkPipelineCache`

Отдельно владеет кэшем `FVkPipeline` для конкретного render-resource owner. Кэш получает shader и готовый layout, а `FVkShader` не владеет pipeline objects и не хранит pipeline cache.

Pipeline cache будет отделён от `FVkShader` в последующем этапе. Кэш должен индексироваться не только render state, но и layout/shader identity, если один shader object может использовать несколько layout configurations.

## Data flow

```text
SPIR-V bytecode
    |
    v
FVkShader
    modules + reflection
    |
    v
FVkPipelineLayout
    descriptor set layouts + push constants + VkPipelineLayout
    |
    v
FVkPipeline
    VkPipeline
```

## Descriptor set rule

Каждый номер `set` в shader interface соответствует одному `VkDescriptorSetLayout` в массиве `VkPipelineLayoutCreateInfo::pSetLayouts`.

```text
shader set N -> pSetLayouts[N]
descriptor set -> отдельно аллоцируется из descriptor pool по pSetLayouts[N]
bind -> firstSet = N
```

Номера set являются позиционными. Если shader использует set 2, но не использует set 0 или set 1, для промежуточных позиций создаются пустые layouts.

## Frequency classification

Frequency classification не входит в текущую ответственность `FVkShader` и не меняет shader `set/binding`.

На следующем этапе она может использоваться как metadata для:

- lifetime descriptor sets;
- частоты обновления ресурсов;
- кэширования и повторного использования descriptor sets;
- выбора момента `vkCmdBindDescriptorSets`.

Неверная classification не должна молча изменять interface, записанный в SPIR-V. При необходимости движок должен выдавать warning или validation error.

## Phase-1 limitations

- One `FVkShader` contains the graphics stages used by the current renderer: vertex, optional geometry, and fragment.
- Combining independently-owned shader objects and compute pipelines are deferred.
- Runtime/unsized descriptor arrays are rejected during reflection; they are not replaced with an arbitrary descriptor count.
- `FVkPipelineLayout` owns its descriptor set layouts and pipeline layout. `FVkPipeline` does not destroy those handles.
- The layout owner must outlive pipelines, descriptor sets, and submitted command buffers that use the layout.

## Initial API direction

```cpp
class FVkShader
{
public:
    void Init(VkDevice device, const ShaderCreateInfo& info);
    const ShaderReflection& GetReflection() const;
};

class FVkPipelineLayout
{
public:
    void Init(VkDevice device, const FVkShader& shader);

    VkPipelineLayout Get() const;
    VkDescriptorSetLayout GetSetLayout(uint32_t set) const;
    const std::vector<VkDescriptorSetLayout>& GetSetLayouts() const;
};
```

API может быть скорректирован при реализации после проверки существующих call sites.

## Out of scope

- frequency classification implementation;
- automatic shader binding remapping;
- material/frame/object descriptor set abstractions;
- advanced descriptor pool strategies;
- descriptor update/binding commands;
- advanced pipeline cache eviction/persistence policies; phase 1 uses an in-memory owner-local cache keyed by render state and pipeline layout.

## Acceptance criteria

- `FVkShader` не создаёт pipeline и pipeline cache.
- Один shader stage может быть отражён независимо от pipeline creation.
- `FVkPipelineLayout` корректно объединяет bindings нескольких stages.
- Конфликт `(set, binding)` с разными type/count диагностируется.
- Для каждого shader set существует соответствующий `VkDescriptorSetLayout`.
- `FVkPipeline` использует готовый pipeline layout и не строит descriptor layouts сам.
- Existing rendering paths can be migrated incrementally without changing shader source bindings.
