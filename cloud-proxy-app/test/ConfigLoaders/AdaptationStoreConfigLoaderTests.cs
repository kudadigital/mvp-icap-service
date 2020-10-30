using Glasswall.IcapServer.CloudProxyApp.ConfigLoaders;
using Glasswall.IcapServer.CloudProxyApp.Configuration;
using Moq;
using NUnit.Framework;

namespace Glasswall.IcapServer.CloudProxyApp.Tests.ConfigLoaders
{
    class AdaptationStoreConfigLoaderTests
    {

        IStoreConfiguration defaultedConfiguration;

        [SetUp]
        public void CreateQueueConfiguration()
        {
            var mockConfiguration = new Mock<IStoreConfiguration>();
            mockConfiguration.SetupAllProperties();

            defaultedConfiguration = AdaptationStoreConfigLoader.SetDefaults(mockConfiguration.Object);
        }

        [Test]
        public void OriginalStorePath_default_is_set()
        {
            // Arrange
            const string DefaultOriginalStorePath = "/var/source";
            // Act

            // Assert
            Assert.That(defaultedConfiguration.OriginalStorePath, Is.EqualTo(DefaultOriginalStorePath), "the set config should match the default");
        }


        [Test]
        public void RebuiltStorePath_default_is_set()
        {
            // Arrange
            const string DefaultRebuiltStorePath = "/var/target";
            // Act

            // Assert
            Assert.That(defaultedConfiguration.RebuiltStorePath, Is.EqualTo(DefaultRebuiltStorePath), "the set config should match the default");
        }

    }
}
