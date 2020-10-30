using Glasswall.IcapServer.CloudProxyApp.ConfigLoaders;
using Glasswall.IcapServer.CloudProxyApp.Configuration;
using Moq;
using NUnit.Framework;
using System;

namespace Glasswall.IcapServer.CloudProxyApp.Tests.ConfigLoaders
{
    class IcapProcessingConfigLoaderTests
    {
        IProcessingConfiguration defaultedConfiguration;

        [SetUp]
        public void CreateQueueConfiguration()
        {
            var mockConfiguration = new Mock<IProcessingConfiguration>();
            mockConfiguration.SetupAllProperties();

            defaultedConfiguration = IcapProcessingConfigLoader.SetDefaults(mockConfiguration.Object);
        }

        [Test]
        public void ProcessingTimeoutDuration_default_is_set()
        {
            // Arrange
            TimeSpan DefaultProcessingTimeoutDuration = TimeSpan.FromMinutes(1);
            // Act

            // Assert
            Assert.That(defaultedConfiguration.ProcessingTimeoutDuration, Is.EqualTo(DefaultProcessingTimeoutDuration), "the set config should match the default");
        }
    }
}
