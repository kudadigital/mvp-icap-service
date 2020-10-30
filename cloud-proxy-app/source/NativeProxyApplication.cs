﻿using Glasswall.IcapServer.CloudProxyApp.AdaptationService;
using Glasswall.IcapServer.CloudProxyApp.Configuration;
using Microsoft.Extensions.Logging;
using System;
using System.IO;
using System.Threading;
using System.Threading.Tasks;

namespace Glasswall.IcapServer.CloudProxyApp
{
    public class NativeProxyApplication
    {
        private readonly IAppConfiguration _appConfiguration;
        private readonly ILogger<NativeProxyApplication> _logger;
        private readonly CancellationTokenSource _processingCancellationTokenSource;
        private readonly TimeSpan _processingTimeoutDuration;
        private readonly string OriginalStorePath;
        private readonly string RebuiltStorePath;

        private readonly IAdaptationServiceClient<AdaptationOutcomeProcessor> _adaptationServiceClient;

        public NativeProxyApplication(IAdaptationServiceClient<AdaptationOutcomeProcessor> adaptationServiceClient,
            IAppConfiguration appConfiguration, IStoreConfiguration storeConfiguration, IProcessingConfiguration processingConfiguration, ILogger<NativeProxyApplication> logger)
        {
            _adaptationServiceClient = adaptationServiceClient ?? throw new ArgumentNullException(nameof(adaptationServiceClient));
            _appConfiguration = appConfiguration ?? throw new ArgumentNullException(nameof(appConfiguration));
            if (storeConfiguration == null) throw new ArgumentNullException(nameof(storeConfiguration));
            if (processingConfiguration == null) throw new ArgumentNullException(nameof(processingConfiguration));
            _logger = logger ?? throw new ArgumentNullException(nameof(logger));
            _processingTimeoutDuration = processingConfiguration.ProcessingTimeoutDuration;
            _processingCancellationTokenSource = new CancellationTokenSource(_processingTimeoutDuration);

            OriginalStorePath = storeConfiguration.OriginalStorePath;
            RebuiltStorePath = storeConfiguration.RebuiltStorePath;
        }

        public Task<int> RunAsync()
        {
            string originalStoreFilePath = string.Empty;
            string rebuiltStoreFilePath = string.Empty;
            var fileId = Guid.NewGuid();
            try
            {
                var processingCancellationToken = _processingCancellationTokenSource.Token;

                _logger.LogInformation($"Using store locations '{OriginalStorePath}' and '{RebuiltStorePath}' for {fileId}");

                originalStoreFilePath = Path.Combine(OriginalStorePath, fileId.ToString());
                rebuiltStoreFilePath = Path.Combine(RebuiltStorePath, fileId.ToString());

                _logger.LogInformation($"Updating 'Original' store for {fileId}");
                File.Copy(_appConfiguration.InputFilepath, originalStoreFilePath, overwrite: true);

                _adaptationServiceClient.Connect();
                var outcome = _adaptationServiceClient.AdaptationRequest(fileId, originalStoreFilePath, rebuiltStoreFilePath, processingCancellationToken);

                if (outcome == ReturnOutcome.GW_REBUILT)
                {
                    _logger.LogInformation($"Copy from '{rebuiltStoreFilePath}' to {_appConfiguration.OutputFilepath}");
                    File.Copy(rebuiltStoreFilePath, _appConfiguration.OutputFilepath, overwrite: true);
                }

                ClearStores(originalStoreFilePath, rebuiltStoreFilePath);

                _logger.LogInformation($"Returning '{outcome}' Outcome for {fileId}");
                return Task.FromResult((int)outcome);
            }
            catch (OperationCanceledException oce)
            {
                _logger.LogError(oce, $"Error Processing Timeout 'input' {fileId} exceeded {_processingTimeoutDuration.TotalSeconds}s");
                ClearStores(originalStoreFilePath, rebuiltStoreFilePath);
                return Task.FromResult((int)ReturnOutcome.GW_ERROR);
            }
            catch (Exception ex)
            {
                _logger.LogError(ex, $"Error Processing 'input' {fileId}");
                ClearStores(originalStoreFilePath, rebuiltStoreFilePath);
                return Task.FromResult((int)ReturnOutcome.GW_ERROR);
            }
        }

        private void ClearStores(string originalStoreFilePath, string rebuiltStoreFilePath)
        {
            try
            {
                _logger.LogInformation($"Clearing stores '{originalStoreFilePath}' and {rebuiltStoreFilePath}");
                if (!string.IsNullOrEmpty(originalStoreFilePath))
                    File.Delete(originalStoreFilePath);
                if (!string.IsNullOrEmpty(rebuiltStoreFilePath))
                    File.Delete(rebuiltStoreFilePath);
            }

            catch (Exception ex)
            {
                _logger.LogWarning(ex, $"Error whilst attempting to clear stores: {ex.Message}");
            }
        }
    }
}
