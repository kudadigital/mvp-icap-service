namespace Glasswall.IcapServer.CloudProxyApp.Configuration
{
    public interface IQueueConfiguration
    {
        public string MBHostName { get; set; }
        public int MBPort { get; set; }
        public string ExchangeName { get; set; }

        public string RequestQueueName { get; set; }
        public string OutcomeQueueName { get; set; }

        public string RequestMessageName { get; set; }

    }
}
