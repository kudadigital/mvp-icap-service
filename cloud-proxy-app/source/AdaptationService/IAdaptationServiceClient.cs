using System;
using System.Threading;

namespace Glasswall.IcapServer.CloudProxyApp.AdaptationService
{
    public interface IAdaptationServiceClient<IResponseProcessor>
    {
        void Connect();
        ReturnOutcome AdaptationRequest(Guid fileId, string originalStoreFilePath, string rebuiltStoreFilePath, CancellationToken processingCancellationToken);
    }
}
