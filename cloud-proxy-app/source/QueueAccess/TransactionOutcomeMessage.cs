using System;

namespace Glasswall.IcapServer.CloudProxyApp.QueueAccess
{
    public class TransactionOutcomeMessage
    {
        public static string Label => "transaction-outcome";

        public Guid FileId { get; internal set; }
        public string FileRebuildSas { get; internal set; }
        internal ReturnOutcome FileOutcome { get; set; }
    }
}
