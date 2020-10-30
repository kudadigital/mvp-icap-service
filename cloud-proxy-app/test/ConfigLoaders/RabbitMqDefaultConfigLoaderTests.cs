using Glasswall.IcapServer.CloudProxyApp.ConfigLoaders;
using Glasswall.IcapServer.CloudProxyApp.Configuration;
using Moq;
using NUnit.Framework;

namespace Glasswall.IcapServer.CloudProxyApp.Tests.ConfigLoaders
{
    class RabbitMqDefaultConfigLoaderTests
    {
        IQueueConfiguration defaultedConfiguration;

        [SetUp]
        public void CreateQueueConfiguration()
        {
            var mockConfiguration = new Mock<IQueueConfiguration>();
            mockConfiguration.SetupAllProperties();

            defaultedConfiguration = RabbitMqDefaultConfigLoader.SetDefaults(mockConfiguration.Object);
        }

        [Test]
        public void ExchangeName_default_is_set()
        {
            // Arrange
            const string DefaultExchangeName = "adaptation-exchange";
            // Act

            // Assert
            Assert.That(defaultedConfiguration.ExchangeName, Is.EqualTo(DefaultExchangeName), "the set config should match the default");
        }

        [Test]
        public void MBHostPort_default_is_set()
        {
            // Arrange
            const int DefaultMBPort = 5672;
            // Act

            // Assert
            Assert.That(defaultedConfiguration.MBPort, Is.EqualTo(DefaultMBPort), "the set config should match the default");
        }

        [Test]
        public void MBHostName_default_is_set()
        {
            // Arrange
            const string DefaultMBHostName = "rabbitmq-service";
            // Act

            // Assert
            Assert.That(defaultedConfiguration.MBHostName, Is.EqualTo(DefaultMBHostName), "the set config should match the default");
        }

        [Test]
        public void RequestQueueName_default_is_set()
        {
            // Arrange
            const string DefaultRequestQueueName = "adaptation-request-queue";
            // Act

            // Assert
            Assert.That(defaultedConfiguration.RequestQueueName, Is.EqualTo(DefaultRequestQueueName), "the set config should match the default");
        }

        [Test]
        public void OutcomeQueueName_default_is_set()
        {
            // Arrange
            const string DefaultOutcomeQueueName = "amq.rabbitmq.reply-to";
            // Act

            // Assert
            Assert.That(defaultedConfiguration.OutcomeQueueName, Is.EqualTo(DefaultOutcomeQueueName), "the set config should match the default");
        }

        [Test]
        public void RequestMessageNamee_default_is_set()
        {
            // Arrange
            const string DefaultRequestMessageName = "adaptation-request";
            // Act

            // Assert
            Assert.That(defaultedConfiguration.RequestMessageName, Is.EqualTo(DefaultRequestMessageName), "the set config should match the default");
        }
    }
}
