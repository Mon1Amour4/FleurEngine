#include "gtest/gtest.h"

#include "LightingSystem.h"

namespace Fleur::Graphics
{
TEST(LightingSystemTests, BuildsFrameDataForEnabledLights)
{
    LightingSystem lighting(128);

    lighting.CreatePointLight(Fleur::Vec3(1.0f, 2.0f, 3.0f), 10.0f, Color::White(), 4.0f);
    lighting.CreateDirectionalLight(Fleur::Vec3(-1.0f, -2.0f, -1.0f), Color::White(), 0.7f);

    const LightingFrameData frameData = lighting.BuildFrameData();

    ASSERT_EQ(frameData.pointLights.size(), 1u);
    EXPECT_FLOAT_EQ(frameData.directionalLight.dirIntens.w, 0.7f);
    EXPECT_FLOAT_EQ(frameData.pointLights[0].radius, 10.0f);
    EXPECT_FLOAT_EQ(frameData.pointLights[0].intensity, 4.0f);
    EXPECT_FLOAT_EQ(frameData.directionalLight.color.x, 1.0f);
}

TEST(LightingSystemTests, AllowsOnlyOneDirectionalLight)
{
    LightingSystem lighting(128);

    const DirectionalLightHandle first =
        lighting.CreateDirectionalLight(Fleur::Vec3(-1.0f, -2.0f, -1.0f), Color::White(), 1.0f);
    const DirectionalLightHandle second =
        lighting.CreateDirectionalLight(Fleur::Vec3(1.0f, 2.0f, 1.0f), Color::Red(), 1.0f);

    EXPECT_TRUE(first.value.IsValid());
    EXPECT_FALSE(second.value.IsValid());
    EXPECT_FLOAT_EQ(lighting.BuildFrameData().directionalLight.dirIntens.w, 1.0f);
}

TEST(LightingSystemTests, DestroyedLightIsNotAggregatedAndHandleCannotBeReused)
{
    LightingSystem lighting(128);
    const PointLightHandle handle = lighting.CreatePointLight(Fleur::Vec3(0.0f), 5.0f, Color::White(), 1.0f);

    lighting.Destroy(handle);

    EXPECT_FALSE(lighting.IsEnabled(handle));
    EXPECT_TRUE(lighting.BuildFrameData().pointLights.empty());
}

TEST(LightingSystemTests, DisabledLightIsNotAggregated)
{
    LightingSystem lighting(128);
    const PointLightHandle handle = lighting.CreatePointLight(Fleur::Vec3(0.0f), 5.0f, Color::White(), 1.0f);

    lighting.Disable(handle);

    EXPECT_FALSE(lighting.IsEnabled(handle));
    EXPECT_TRUE(lighting.BuildFrameData().pointLights.empty());
}

TEST(LightingSystemTests, PointLightCanBeCreatedDisabled)
{
    LightingSystem lighting(128);
    const PointLightHandle handle = lighting.CreatePointLight(Fleur::Vec3(0.0f), 5.0f, Color::White(), 1.0f, false);

    EXPECT_FALSE(lighting.IsEnabled(handle));
    EXPECT_TRUE(lighting.BuildFrameData().pointLights.empty());
}

TEST(LightingSystemTests, ReusesDestroyedSlotWithNewGeneration)
{
    LightingSystem lighting(128);
    const PointLightHandle oldHandle = lighting.CreatePointLight(Fleur::Vec3(0.0f), 5.0f, Color::White(), 1.0f);

    lighting.Destroy(oldHandle);
    const PointLightHandle newHandle = lighting.CreatePointLight(Fleur::Vec3(1.0f), 5.0f, Color::White(), 1.0f);

    EXPECT_EQ(newHandle.value.index, oldHandle.value.index);
    EXPECT_NE(newHandle.value.generation, oldHandle.value.generation);
    EXPECT_FALSE(lighting.IsEnabled(oldHandle));
    EXPECT_TRUE(lighting.IsEnabled(newHandle));
}

TEST(LightingSystemTests, EnforcesConfiguredPointLightCapacity)
{
    LightingSystem lighting(1);

    const PointLightHandle first = lighting.CreatePointLight(Fleur::Vec3(0.0f), 5.0f, Color::White(), 1.0f);
    const PointLightHandle second = lighting.CreatePointLight(Fleur::Vec3(1.0f), 5.0f, Color::White(), 1.0f);

    EXPECT_TRUE(first.value.IsValid());
    EXPECT_FALSE(second.value.IsValid());
    EXPECT_EQ(lighting.BuildFrameData().pointLights.size(), 1u);
}

TEST(LightingSystemTests, OwnsPointLightDirtyState)
{
    LightingSystem lighting(128);
    EXPECT_TRUE(lighting.BuildFrameData().pointLightsDirty);

    lighting.ClearPointLightsDirty();
    EXPECT_FALSE(lighting.BuildFrameData().pointLightsDirty);

    lighting.CreatePointLight(Fleur::Vec3(0.0f), 5.0f, Color::White(), 1.0f);
    EXPECT_TRUE(lighting.BuildFrameData().pointLightsDirty);
}
}  // namespace Fleur::Graphics
