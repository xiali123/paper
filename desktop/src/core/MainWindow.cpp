#include "core/MainWindow.hpp"
#include "search/SearchWidget.hpp"
#include "latex/LatexEditorWidget.hpp"
#include "latex/LatexPreviewWidget.hpp"
#include "tools/ResultView.hpp"
#include "tools/ProgressView.hpp"
#include "search/FilterPanel.hpp"
#include "core/ThemeManager.hpp"
#include "tools/HeroWidget.hpp"
#include "tools/FeatureCards.hpp"
#include "paper/PaperCardView.hpp"
#include "core/ApiManager.hpp"
#include "core/PaperCache.hpp"
#include "workspace/ExportManager.hpp"
#include "core/AuthManager.hpp"
#include "core/LoginWindow.hpp"
#include "paper/PaperDetailDialog.hpp"
#include "core/SettingsDialog.hpp"
#include "paper/FavoriteManager.hpp"
#include "search/SearchHistory.hpp"
#include "database/LocalDatabase.hpp"
#include "core/ToastWidget.hpp"
#include "paper/PaperNotesDialog.hpp"
#include "reading/ReadingListManager.hpp"
#include "search/AdvancedSearchDialog.hpp"
#include "paper/OfflineCacheManager.hpp"
#include "paper/PaperCompareDialog.hpp"
#include "paper/RecentHistoryWidget.hpp"
#include "paper/BatchOperationsBar.hpp"
#include "core/SystemTrayManager.hpp"
#include "tools/WelcomeWidget.hpp"
#include "citation/CitationGraphWidget.hpp"
#include "paper/PaperTimelineWidget.hpp"
#include "core/ShortcutConfigDialog.hpp"
#include "core/NotificationCenter.hpp"
#include "core/DragDropHandler.hpp"
#include "paper/DoiLookupDialog.hpp"
#include "paper/PaperDeduplicator.hpp"
#include "tools/MarkdownNoteEditor.hpp"
#include "core/CommandPalette.hpp"
#include "visualization/TagCloudWidget.hpp"
#include "core/UpdateChecker.hpp"
#include "core/SessionManager.hpp"
#include "tools/SplitPaperView.hpp"
#include "paper/RatingWidget.hpp"
#include "search/SearchSuggestWidget.hpp"
#include "core/ContextMenuBuilder.hpp"
#include "paper/PaperExportDialog.hpp"
#include "core/ThemeCustomizer.hpp"
#include "search/FilterChipBar.hpp"
#include "visualization/PaperStatsChart.hpp"
#include "tools/QuickNoteWidget.hpp"
#include "workspace/PaperCollectionWidget.hpp"
#include "tools/SideBySideDiff.hpp"
#include "tools/ProgressTracker.hpp"
#include "workspace/BackupRestoreWidget.hpp"
#include "visualization/AdvancedTableWidget.hpp"
#include "paper/PaperRecommendationEngine.hpp"
#include "pdf/PdfThumbnailWidget.hpp"
#include "workspace/BatchImportWidget.hpp"
#include "workspace/TagManager.hpp"
#include "workspace/WorkspaceManager.hpp"
#include "visualization/AnnotationWidget.hpp"
#include "paper/PaperVersionHistory.hpp"
#include "paper/PaperRankingWidget.hpp"
#include "analysis/AiSummarizerWidget.hpp"
#include "workspace/JournalBrowserWidget.hpp"
#include "paper/PaperFeedWidget.hpp"
#include "core/HotkeyManager.hpp"
#include "citation/CitationExporter.hpp"
#include "reading/ReadingQueueWidget.hpp"
#include "workspace/CollaborationWidget.hpp"
#include "visualization/PaperComparisonMatrix.hpp"
#include "pdf/PdfViewerWidget.hpp"
#include "search/SmartSearchWidget.hpp"
#include "analysis/PaperClusteringWidget.hpp"
#include "reading/ScheduledTaskWidget.hpp"
#include "workspace/UserProfileWidget.hpp"
#include "tools/MiniBrowserWidget.hpp"
#include "analysis/PaperSimilarityWidget.hpp"
#include "workspace/ExportTemplateManager.hpp"
#include "tools/KeyboardMacroWidget.hpp"
#include "tools/PluginLoaderWidget.hpp"
#include "tools/WidgetGallery.hpp"
#include "visualization/PaperMindMapWidget.hpp"
#include "workspace/BatchDownloadWidget.hpp"
#include "reading/ReadingTimerWidget.hpp"
#include "paper/PaperGraderWidget.hpp"
#include "visualization/DataVisualizationWidget.hpp"
#include "tools/PaperQuizWidget.hpp"
#include "tools/ClipboardHistoryWidget.hpp"
#include "tools/PaperTranslatorWidget.hpp"
#include "citation/BibliographyBuilderWidget.hpp"
#include "analysis/LanguageDetectorWidget.hpp"
#include "visualization/PaperStoryboardWidget.hpp"
#include "tools/MarkdownPreviewWidget.hpp"
#include "search/SearchQueryBuilder.hpp"
#include "tools/PaperReportGenerator.hpp"
#include "visualization/PaperNetworkGraph.hpp"
#include "tools/PaperChecklistWidget.hpp"
#include "tools/SessionStatisticsWidget.hpp"
#include "tools/ColorSchemeEditor.hpp"
#include "paper/PaperMergerWidget.hpp"
#include "analysis/AbstractSummaryWidget.hpp"
#include "visualization/PaperDependencyWidget.hpp"
#include "pdf/PdfBookmarkWidget.hpp"
#include "visualization/PaperComparisonSlider.hpp"
#include "tools/NotificationRuleEditor.hpp"
#include "visualization/PaperTimelineBuilder.hpp"
#include "workspace/PaperCommentWidget.hpp"
#include "workspace/PaperShareWidget.hpp"
#include "workspace/PaperEmbeddingWidget.hpp"
#include "reading/ReadingSchedulerWidget.hpp"
#include "workspace/PaperTemplateLibrary.hpp"
#include "visualization/PaperAnnotationHighlighter.hpp"
#include "citation/CitationNetworkVisualizer.hpp"
#include "visualization/PaperBookmarkSync.hpp"
#include "reading/ReadingProgressDashboard.hpp"
#include "visualization/PaperRatingChart.hpp"
#include "workspace/PaperWorkflowAutomator.hpp"
#include "citation/CitationGraphExplorer.hpp"
#include "analysis/PaperInsightExtractor.hpp"
#include "reading/ReadingJournalWidget.hpp"
#include "citation/PaperCrossReference.hpp"
#include "analysis/PaperPeerReviewer.hpp"
#include "visualization/LiteratureMatrixWidget.hpp"
#include "visualization/PaperConceptMap.hpp"
#include "reading/ReadingStreakTracker.hpp"
#include "workspace/PaperExportBatch.hpp"
#include "analysis/PaperSummarizerChain.hpp"
#include "analysis/ResearchTrendAnalyzer.hpp"
#include "analysis/PaperDuplicateDetector.hpp"
#include "reading/ReadingGoalTracker.hpp"
#include "visualization/PaperKnowledgeBase.hpp"
#include "citation/PaperReferenceExtractor.hpp"
#include "search/SearchHistoryAnalyzer.hpp"
#include "citation/PaperCitationStyleEditor.hpp"
#include "reading/ReadingSessionLog.hpp"
#include "visualization/PaperFigureExtractor.hpp"
#include "analysis/PaperTopicModeler.hpp"
#include "reading/ReadingSpeedAnalyzer.hpp"
#include "citation/PaperCitationCounter.hpp"
#include "analysis/PaperKeywordExtractor.hpp"
#include "reading/ReadingPlanWidget.hpp"
#include "workspace/PaperCollaborationBoard.hpp"
#include "workspace/ResearchNoteWidget.hpp"
#include "workspace/PaperFeedbackCollector.hpp"
#include "visualization/LiteratureTimelineWidget.hpp"
#include "visualization/PaperTagCloudWidget.hpp"
#include "reading/PaperReadingHeatmap.hpp"
#include "citation/CitationStyleGenerator.hpp"
#include "workspace/PaperVersionDiff.hpp"
#include "reading/ResearchCalendarWidget.hpp"
#include "analysis/PaperSentimentAnalyzer.hpp"
#include "citation/PaperCitationNetwork.hpp"
#include "reading/PaperCoReadingTracker.hpp"
#include "analysis/PaperQaChatWidget.hpp"
#include "visualization/PaperTrendHeatmap.hpp"
#include "analysis/PaperAutoTagger.hpp"
#include "paper/PaperReadingScoreWidget.hpp"
#include "tools/PaperMathFormulaRenderer.hpp"
#include "workspace/PaperReadingGroupWidget.hpp"
#include "tools/PaperAlertMonitor.hpp"
#include "visualization/PaperGeographyMap.hpp"
#include "visualization/PaperCitationRadarChart.hpp"
#include "visualization/PaperResearchTimeline.hpp"
#include "analysis/PaperArgumentParser.hpp"
#include "tools/PaperCodeSnippetManager.hpp"
#include "workspace/PaperResourceAllocator.hpp"
#include "visualization/PaperInfluenceGraph.hpp"
#include "analysis/PaperMethodologyExtractor.hpp"
#include "tools/PaperDataTransformWidget.hpp"
#include "workspace/PaperExperimentTracker.hpp"
#include "reading/PaperReadingNotesOrganizer.hpp"
#include "visualization/PaperCollaborationHeatmap.hpp"
#include "analysis/PaperBiasDetector.hpp"
#include "tools/PaperValidationWidget.hpp"
#include "workspace/PaperGrantTracker.hpp"
#include "reading/PaperReadingChallengeWidget.hpp"
#include "visualization/PaperCitationTreeMap.hpp"
#include "analysis/PaperReproducibilityChecker.hpp"
#include "tools/PaperPomodoroTimer.hpp"
#include "workspace/PaperDatasetManager.hpp"
#include "reading/PaperReadingAnalyticsWidget.hpp"
#include "visualization/PaperWorkflowSankey.hpp"
#include "analysis/PaperEntityExtractor.hpp"
#include "tools/PaperSideBySideCompare.hpp"
#include "workspace/PaperConferencePlanner.hpp"
#include "citation/PaperPeerReviewTracker.hpp"
#include "tools/PaperSurveyBuilder.hpp"
#include "analysis/PaperCitationContextAnalyzer.hpp"
#include "reading/PaperReadingPathOptimizer.hpp"
#include "visualization/PaperKnowledgeGraphExplorer.hpp"
#include "citation/PaperAuthorDisambiguator.hpp"
#include "analysis/PaperCitationTimingAnalyzer.hpp"
#include "reading/PaperReadingListSorter.hpp"
#include "visualization/PaperTrendPredictor.hpp"
#include "workspace/PaperCollaborationMatcher.hpp"
#include "tools/PaperFormulaSearchEngine.hpp"
#include "analysis/PaperRecommenderFeedback.hpp"
#include "reading/PaperReadingSpeedTracker.hpp"
#include "visualization/PaperCitationNetworkD3.hpp"
#include "workspace/PaperProjectTimelineWidget.hpp"
#include "tools/PaperLatexSymbolFinder.hpp"
#include "analysis/PaperArgumentStrengthAnalyzer.hpp"
#include "reading/PaperReadingMoodTracker.hpp"
#include "visualization/PaperCitationImpactCloud.hpp"
#include "workspace/PaperBudgetPlanner.hpp"
#include "tools/PaperBatchRenamer.hpp"
#include "analysis/PaperSemanticSimilarityMatrix.hpp"
#include "reading/PaperReadingRetentionTracker.hpp"
#include "visualization/PaperCitationFlowDiagram.hpp"
#include "workspace/PaperEthicsChecker.hpp"
#include "tools/PaperRegexSearchTool.hpp"
#include "analysis/PaperContradictionDetector.hpp"
#include "reading/PaperAnnotationSync.hpp"
#include "visualization/PaperResearchHeatmap.hpp"
#include "workspace/PaperCollaborationScore.hpp"
#include "tools/PaperMarkdownExporter.hpp"
#include "analysis/PaperClaimStrengthAnalyzer.hpp"
#include "reading/PaperReadingSessionTimer.hpp"
#include "visualization/PaperKeywordEvolution.hpp"
#include "workspace/PaperReviewAssignment.hpp"
#include "tools/PaperTableExtractor.hpp"
#include "analysis/PaperMethodologyComparator.hpp"
#include "reading/PaperReadingSpeedTest.hpp"
#include "visualization/PaperCitationSankey.hpp"
#include "workspace/PaperProjectMilestone.hpp"
#include "tools/PaperFigureCaptioner.hpp"
#include "analysis/PaperEvidenceStrength.hpp"
#include "reading/PaperVocabularyTracker.hpp"
#include "visualization/PaperTrendSparkline.hpp"
#include "workspace/PaperTeamAvailability.hpp"
#include "tools/PaperPDFAnnotator.hpp"
#include "analysis/PaperNoveltyScorer.hpp"
#include "reading/PaperReadingGoalWidget.hpp"
#include "visualization/PaperImpactTreemap.hpp"
#include "workspace/PaperBudgetForecast.hpp"
#include "tools/PaperCodeSnippetExtractor.hpp"
#include "analysis/PaperClaimVerifier.hpp"
#include "reading/PaperReadingMilestone.hpp"
#include "visualization/PaperDonutChart.hpp"
#include "workspace/PaperInvoiceTracker.hpp"
#include "tools/PaperApiTester.hpp"
#include "analysis/PaperEvidenceChain.hpp"
#include "reading/PaperReadingChallenge.hpp"
#include "visualization/PaperBubbleChart.hpp"
#include "workspace/PaperExpenseLogger.hpp"
#include "tools/PaperWebScraper.hpp"
#include "analysis/PaperHypothesisTracker.hpp"
#include "reading/PaperReadingPuzzle.hpp"
#include "visualization/PaperRadarChart.hpp"
#include "workspace/PaperContractManager.hpp"
#include "tools/PaperCronScheduler.hpp"
#include "analysis/PaperArgumentMapper.hpp"
#include "reading/PaperReadingBadge.hpp"
#include "visualization/PaperHeatmapGrid.hpp"
#include "workspace/PaperProposalTracker.hpp"
#include "tools/PaperLogViewer.hpp"
#include "analysis/PaperFallacyDetector.hpp"
#include "reading/PaperReadingStreakBoard.hpp"
#include "visualization/PaperScatterPlot.hpp"
#include "workspace/PaperVendorRating.hpp"
#include "tools/PaperConfigEditor.hpp"
#include "analysis/PaperBiasChecker.hpp"
#include "reading/PaperReadingGamification.hpp"
#include "visualization/PaperContourPlot.hpp"
#include "workspace/PaperLicenseTracker.hpp"
#include "tools/PaperEnvironmentSwitcher.hpp"
#include "analysis/PaperSentimentTracker.hpp"
#include "reading/PaperReadingAchievement.hpp"
#include "visualization/PaperWaterfallChart.hpp"
#include "workspace/PaperComplianceMonitor.hpp"
#include "tools/PaperLogAnalyzer.hpp"
#include "analysis/PaperTopicClusterer.hpp"
#include "reading/PaperReadingDashboard.hpp"
#include "visualization/PaperFunnelChart.hpp"
#include "workspace/PaperAuditLogger.hpp"
#include "tools/PaperHealthMonitor.hpp"
#include "analysis/PaperArgumentStrength.hpp"
#include "reading/PaperReadingScorecard.hpp"
#include "visualization/PaperGaugeChart.hpp"
#include "workspace/PaperPolicyManager.hpp"
#include "tools/PaperCacheInspector.hpp"
#include "analysis/PaperConceptExtractor.hpp"
#include "reading/PaperReadingHeatmapGrid.hpp"
#include "visualization/PaperSankeyDiagram.hpp"
#include "workspace/PaperRiskAssessor.hpp"
#include "tools/PaperFeatureFlagManager.hpp"
#include "analysis/PaperEntityRecognizer.hpp"
#include "reading/PaperReadingProgressGrid.hpp"
#include "visualization/PaperTreemapChart.hpp"
#include "workspace/PaperCostEstimator.hpp"
#include "tools/PaperPluginInstaller.hpp"
#include "analysis/PaperArgumentVisualizer.hpp"
#include "reading/PaperReadingScorePredictor.hpp"
#include "visualization/PaperWordCloud.hpp"
#include "workspace/PaperVendorComparison.hpp"
#include "tools/PaperDeploymentTracker.hpp"
#include "analysis/PaperCitationAnalyzer.hpp"
#include "reading/PaperReadingCluster.hpp"
#include "visualization/PaperHeatmapWidget.hpp"
#include "workspace/PaperSprintBoard.hpp"
#include "tools/PaperKeyboardShortcut.hpp"
#include "analysis/PaperClaimTracker.hpp"
#include "reading/PaperReadingCalendar.hpp"
#include "visualization/PaperNetworkGraph3D.hpp"
#include "workspace/PaperTimeTracker.hpp"
#include "tools/PaperThemeBuilder.hpp"
#include "analysis/PaperSentimentExplorer.hpp"
#include "reading/PaperReadingCompetency.hpp"
#include "visualization/PaperParallelCoordinates.hpp"
#include "workspace/PaperResourceScheduler.hpp"
#include "tools/PaperScriptRunner.hpp"
#include "analysis/PaperTopicEvolution.hpp"
#include "reading/PaperReadingFlow.hpp"
#include "visualization/PaperViolinPlot.hpp"
#include "workspace/PaperMeetingScheduler.hpp"
#include "tools/PaperContextMenuEditor.hpp"
#include "analysis/PaperArgumentMiner.hpp"
#include "reading/PaperReadingWorkshop.hpp"
#include "visualization/PaperRadarSpinner.hpp"
#include "workspace/PaperInventoryTracker.hpp"
#include "tools/PaperNotificationFilter.hpp"
#include "analysis/PaperEvidenceEvaluator.hpp"
#include "reading/PaperAnnotationStudio.hpp"
#include "visualization/PaperFlowDiagram.hpp"
#include "workspace/PaperFundingMonitor.hpp"
#include "tools/PaperUpdateChecker.hpp"
#include "analysis/PaperBiasScanner.hpp"
#include "reading/PaperCitationLogger.hpp"
#include "visualization/PaperBubbleMatrix.hpp"
#include "workspace/PaperTaskAutomator.hpp"
#include "tools/PaperLogInspector.hpp"
#include "analysis/PaperConceptGap.hpp"
#include "reading/PaperSkillAssessor.hpp"
#include "visualization/PaperGanttChart.hpp"
#include "workspace/PaperPaperRecommender.hpp"
#include "tools/PaperSnippetVault.hpp"
#include "analysis/PaperNoveltyScore.hpp"
#include "reading/PaperStudyPlanner.hpp"
#include "visualization/PaperDotPlot.hpp"
#include "workspace/PaperLabNotebook.hpp"
#include "tools/PaperVersionTree.hpp"
#include "analysis/PaperClaimValidator.hpp"
#include "reading/PaperReadingJournal.hpp"
#include "visualization/PaperStackedChart.hpp"
#include "workspace/PaperTeamWorkspace.hpp"
#include "tools/PaperMarkupEditor.hpp"
#include "analysis/PaperCohortAnalyzer.hpp"
#include "reading/PaperFlashcardDeck.hpp"
#include "visualization/PaperRingChart.hpp"
#include "workspace/PaperMilestoneTracker.hpp"
#include "tools/PaperPluginManager.hpp"
#include "analysis/PaperContradictionFinder.hpp"
#include "reading/PaperSpacedRepetition.hpp"
#include "visualization/PaperAreaChart.hpp"
#include "workspace/PaperRiskMatrix.hpp"
#include "tools/PaperMacroRecorder.hpp"
#include "analysis/PaperHypothesisTester.hpp"
#include "reading/PaperVocabularyBuilder.hpp"
#include "visualization/PaperCandlestickChart.hpp"
#include "workspace/PaperBudgetTracker.hpp"
#include "tools/PaperColorPicker.hpp"
#include "analysis/PaperCorrelationEngine.hpp"
#include "reading/PaperQuizGenerator.hpp"
#include "visualization/PaperPyramidChart.hpp"
#include "workspace/PaperTeamScoreboard.hpp"
#include "tools/PaperFontBrowser.hpp"
#include "analysis/PaperFactorAnalyzer.hpp"
#include "reading/PaperNoteOrganizer.hpp"
#include "visualization/PaperBulletChart.hpp"
#include "workspace/PaperAssetManager.hpp"
#include "tools/PaperRegexTester.hpp"
#include "analysis/PaperOutlierDetector.hpp"
#include "reading/PaperPaperTimeline.hpp"
#include "visualization/PaperLollipopChart.hpp"
#include "workspace/PaperConferenceTracker.hpp"
#include "tools/PaperDiffViewer.hpp"
#include "analysis/PaperClusterValidator.hpp"
#include "reading/PaperReferenceGraph.hpp"
#include "visualization/PaperHistogramWidget.hpp"
#include "workspace/PaperExperimentLogger.hpp"
#include "tools/PaperArchiveManager.hpp"
#include "analysis/PaperTopicTracker.hpp"
#include "reading/PaperReadingThermometer.hpp"
#include "visualization/PaperStepChart.hpp"
#include "workspace/PaperProjectBoard.hpp"
#include "tools/PaperClipboardManager.hpp"
#include "analysis/PaperAnomalyDetector.hpp"
#include "reading/PaperReadingRadar.hpp"
#include "visualization/PaperTreemapWidget.hpp"
#include "workspace/PaperSurveyManager.hpp"
#include "tools/PaperBookmarkOrganizer.hpp"
#include "analysis/PaperPatternMiner.hpp"
#include "reading/PaperReadingPulse.hpp"
#include "visualization/PaperRadialChart.hpp"
#include "workspace/PaperCollaborationHub.hpp"
#include "tools/PaperCodeVault.hpp"
#include "analysis/PaperBiasMonitor.hpp"
#include "reading/PaperReadingVitals.hpp"
#include "visualization/PaperWaffleChart.hpp"
#include "workspace/PaperTeamSync.hpp"
#include "tools/PaperFormulaEditor.hpp"
#include "analysis/PaperCausalFinder.hpp"
#include "reading/PaperReadingCompass.hpp"
#include "visualization/PaperMarimekkoChart.hpp"
#include "workspace/PaperResourcePool.hpp"
#include "tools/PaperUnitConverter.hpp"
#include "analysis/PaperRelevanceScorer.hpp"
#include "reading/PaperReadingSprint.hpp"
#include "visualization/PaperParallelPlot.hpp"
#include "workspace/PaperVersionTracker.hpp"
#include "tools/PaperTimerWidget.hpp"
#include "analysis/PaperSemanticMapper.hpp"
#include "reading/PaperReadingWaves.hpp"
#include "visualization/PaperDendrogramView.hpp"
#include "workspace/PaperWorkflowBoard.hpp"
#include "tools/PaperChecksumTool.hpp"
#include "analysis/PaperEmbeddingVisualizer.hpp"
#include "reading/PaperReadingGradient.hpp"
#include "visualization/PaperHeatmapMatrix.hpp"
#include "workspace/PaperPipelineView.hpp"
#include "tools/PaperDiffTool.hpp"
#include "analysis/PaperKnowledgeExtractor.hpp"
#include "reading/PaperReadingElevation.hpp"
#include "visualization/PaperBoxPlotWidget.hpp"
#include "workspace/PaperTimelineGantt.hpp"
#include "tools/PaperFileWatcher.hpp"
#include "analysis/PaperNoveltyScore.hpp"
#include "reading/PaperStudyPlanner.hpp"
#include "visualization/PaperDotPlot.hpp"
#include "workspace/PaperLabNotebook.hpp"
#include "tools/PaperVersionTree.hpp"
#include "analysis/PaperContrastAnalyzer.hpp"
#include "reading/PaperReadingGoalTree.hpp"
#include "visualization/PaperLollipopChart.hpp"
#include "workspace/PaperExperimentTracker.hpp"
#include "tools/PaperLogRotator.hpp"
#include "analysis/PaperClaimVerifier.hpp"
#include "reading/PaperReadingHeatmap.hpp"
#include "visualization/PaperRangeChart.hpp"
#include "workspace/PaperProtocolManager.hpp"
#include "tools/PaperConfigDiffer.hpp"
#include "analysis/PaperEvidenceWeigher.hpp"
#include "reading/PaperReadingDepth.hpp"
#include "visualization/PaperBulletChart.hpp"
#include "workspace/PaperHypothesisBoard.hpp"
#include "tools/PaperPatchManager.hpp"
#include "analysis/PaperBiasDetector.hpp"
#include "reading/PaperReadingScorecard.hpp"
#include "visualization/PaperFunnelChart.hpp"
#include "workspace/PaperTaskBoard.hpp"
#include "tools/PaperTokenCounter.hpp"
#include "analysis/PaperReproducibilityChecker.hpp"
#include "reading/PaperReadingPace.hpp"
#include "visualization/PaperCandlestickChart.hpp"
#include "workspace/PaperMilestoneTracker.hpp"
#include "tools/PaperServiceMonitor.hpp"
#include "analysis/PaperArgumentStrength.hpp"
#include "reading/PaperReadingJournal.hpp"
#include "visualization/PaperWaterfallChart.hpp"
#include "workspace/PaperReleasePlanner.hpp"
#include "tools/PaperDeprecationTracker.hpp"
#include "analysis/PaperCitationVerifier.hpp"
#include "reading/PaperReadingBacklog.hpp"
#include "visualization/PaperRadarChart.hpp"
#include "workspace/PaperSprintBoard.hpp"
#include "tools/PaperKeyStore.hpp"
#include "analysis/PaperImpactPredictor.hpp"
#include "reading/PaperReadingVelocity.hpp"
#include "visualization/PaperSankeyChart.hpp"
#include "workspace/PaperRetrospective.hpp"
#include "tools/PaperSecretVault.hpp"
#include "analysis/PaperTrendDetector.hpp"
#include "reading/PaperReadingBinge.hpp"
#include "visualization/PaperTornadoChart.hpp"
#include "workspace/PaperCapacityPlanner.hpp"
#include "tools/PaperFeatureFlag.hpp"
#include "citation/PaperCitationNetwork.hpp"
#include "reading/PaperReadingStreak.hpp"
#include "visualization/PaperViolinChart.hpp"
#include "workspace/PaperDependencyGraph.hpp"
#include "tools/PaperAuditLog.hpp"
#include "analysis/PaperSemanticsMapper.hpp"
#include "reading/PaperReadingCalendar.hpp"
#include "visualization/PaperChordDiagram.hpp"
#include "workspace/PaperStandupTracker.hpp"
#include "tools/PaperRateLimiter.hpp"
#include "analysis/PaperEntityLinker.hpp"
#include "reading/PaperReadingDigest.hpp"
#include "visualization/PaperJoyPlot.hpp"
#include "workspace/PaperIncidentBoard.hpp"
#include "tools/PaperHealthCheck.hpp"
#include "analysis/PaperTopicCluster.hpp"
#include "reading/PaperReadingRhythm.hpp"
#include "visualization/PaperPolarChart.hpp"
#include "workspace/PaperOnCallScheduler.hpp"
#include "tools/PaperCacheWarmer.hpp"
#include "analysis/PaperSemanticShift.hpp"
#include "reading/PaperReadingSprintTimer.hpp"
#include "visualization/PaperSwarmPlot.hpp"
#include "workspace/PaperTaskDependency.hpp"
#include "tools/PaperLatencyProfiler.hpp"
#include "analysis/PaperConceptDrift.hpp"
#include "reading/PaperReadingZoneTracker.hpp"
#include "visualization/PaperRidgelinePlot.hpp"
#include "workspace/PaperMeetingNotes.hpp"
#include "tools/PaperSchemaMigrator.hpp"
#include "analysis/PaperTopicSentiment.hpp"
#include "reading/PaperReadingFlowState.hpp"
#include "visualization/PaperHexbinPlot.hpp"
#include "workspace/PaperCodeReviewTracker.hpp"
#include "tools/PaperApiVersionChecker.hpp"
#include "citation/PaperCitationGraph.hpp"
#include "reading/PaperReadingSessionSync.hpp"
#include "visualization/PaperStreamgraphPlot.hpp"
#include "workspace/PaperSprintRetroBoard.hpp"
#include "tools/PaperEndpointMonitor.hpp"
#include "analysis/PaperTopicClusterer2.hpp"
#include "reading/PaperReadingStaminaTracker.hpp"
#include "visualization/PaperClevelandDotPlot.hpp"
#include "workspace/PaperWikiEditor.hpp"
#include "tools/PaperCertManager.hpp"
#include "analysis/PaperArgumentEvaluator.hpp"
#include "reading/PaperReadingBookmarks.hpp"
#include "visualization/PaperMarimekkoChart2.hpp"
#include "workspace/PaperAssetInventory.hpp"
#include "tools/PaperSecretScanner.hpp"
#include "citation/PaperCitationMatcher.hpp"
#include "reading/PaperReadingHighlight.hpp"
#include "visualization/PaperWaterfallChart2.hpp"
#include "workspace/PaperTeamCalendar.hpp"
#include "tools/PaperDnsLookupTool.hpp"
#include <QTimer>
#include <QCloseEvent>
#include <QResizeEvent>
#include <QPainter>
#include <QLinearGradient>
#include <QScrollArea>

#include <QMenuBar>
#include <QShortcut>
#include <QComboBox>
#include <QCheckBox>
#include <QLineEdit>
#include <QTextEdit>
#include <QTableWidget>
#include <QToolBar>
#include <QStatusBar>
#include <QMessageBox>
#include <QFileDialog>
#include <QInputDialog>
#include <QFile>
#include <QSettings>
#include <QApplication>
#include <QStyleFactory>
#include <QThread>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSplitter>
#include <QLabel>
#include <QPushButton>
#include <QDebug>
#include <functional>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent) {

    // Initialize theme manager first
    themeManager_ = new ThemeManager(this);
    themeManager_->applyTheme();

    // Initialize API manager
    apiManager_ = new ApiManager(this);

    // Initialize paper cache for pagination optimization
    paperCache_ = new PaperCache(this);
    paperCache_->setMaxCachePages(20);

    // Initialize favorite manager and search history
    auto* favoriteManager = new FavoriteManager(this);
    auto* searchHistory = new SearchHistory(this);

    // Initialize export manager
    exportManager_ = new ExportManager(this);
    connect(exportManager_, &ExportManager::exportCompleted,
            this, [this](const QString& fileName, int count) {
        QMessageBox::information(this, "导出成功",
            QString("成功导出 %1 篇论文到:\n%2").arg(count).arg(fileName));
        statusBar()->showMessage("导出成功: " + fileName, 5000);
    });
    connect(exportManager_, &ExportManager::exportFailed,
            this, [this](const QString& error) {
        QMessageBox::warning(this, "导出失败", error);
    });

    localDb_ = new LocalDatabase(this);
    if (!localDb_->open()) {
        qDebug() << "Local database unavailable";
        localDb_->close();
    }

    setWindowTitle("📚 PaperCrawler - Academic Paper Search Tool");
    resize(1400, 900);  // Increased from 1200x800 for better display

    setupUI();
    createMenus();
    createToolBar();
    createThemeButton();
    connectSignals();
    loadSettings();

    initializeAuthentication();

    // Keyboard shortcuts
    auto* searchShortcut = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_F), this);
    connect(searchShortcut, &QShortcut::activated, this, [this]() {
        searchWidget_->setFocus();
    });
    auto* exportShortcut = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_E), this);
    connect(exportShortcut, &QShortcut::activated, this, &MainWindow::onExport);
    auto* themeShortcut = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_T), this);
    connect(themeShortcut, &QShortcut::activated, this, &MainWindow::onToggleTheme);
    auto* statsShortcut = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_U), this);
    connect(statsShortcut, &QShortcut::activated, this, &MainWindow::onShowStatistics);
    auto* settingsShortcut = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_Comma), this);
    connect(settingsShortcut, &QShortcut::activated, this, &MainWindow::onPreferences);

    // Tab switch shortcuts Ctrl+1..8
    for (int i = 1; i <= 8; ++i) {
        auto* sc = new QShortcut(QKeySequence(Qt::CTRL | (Qt::Key_1 + i - 1)), this);
        connect(sc, &QShortcut::activated, this, [this, i]() {
            if (tabWidget_ && i < tabWidget_->count()) tabWidget_->setCurrentIndex(i);
        });
    }

    // Check API health on startup
    healthTimer_.start();
    apiManager_->checkHealth();

    // Periodic health check every 30 seconds
    auto* healthCheckTimer = new QTimer(this);
    connect(healthCheckTimer, &QTimer::timeout, this, [this]() {
        healthTimer_.restart();
        apiManager_->checkHealth();
    });
    healthCheckTimer->start(30000);

    statusBar()->showMessage("Ready - Ctrl+F: Search | Ctrl+E: Export | Ctrl+T: Theme | Ctrl+,: Settings", 5000);

    // System tray icon
    trayIcon_ = new QSystemTrayIcon(this);
    trayIcon_->setToolTip("PaperCrawler Desktop");
    trayIcon_->setIcon(windowIcon());

    auto* trayMenu = new QMenu(this);
    trayMenu->addAction("Show", this, &MainWindow::showNormal);
    trayMenu->addAction("Search", this, [this]() {
        showNormal();
        searchWidget_->setFocus();
    });
    trayMenu->addSeparator();
    trayMenu->addAction("Quit", QApplication::quit);
    trayIcon_->setContextMenu(trayMenu);

    connect(trayIcon_, &QSystemTrayIcon::activated, this, [this](QSystemTrayIcon::ActivationReason reason) {
        if (reason == QSystemTrayIcon::DoubleClick) {
            showNormal();
            activateWindow();
        }
    });

    trayIcon_->show();
}

MainWindow::~MainWindow() {
    saveSettings();
    // Qt parent-child mechanism auto-deletes children
}

void MainWindow::setupUI() {
    // Tab widget as central widget
    tabWidget_ = new QTabWidget(this);
    tabWidget_->setTabPosition(QTabWidget::South);
    tabWidget_->setDocumentMode(true);
    tabWidget_->setStyleSheet(
        "QTabWidget::pane { border: none; }"
        "QTabBar::tab { padding: 10px 28px; font-weight: bold; font-size: 12px; "
        "  border: none; border-top: 2px solid transparent; }"
        "QTabBar::tab:selected { color: #4f46e5; border-top: 2px solid #4f46e5; }"
        "QTabBar::tab:hover { color: #6366f1; background: #f8fafc; }"
    );
    setCentralWidget(tabWidget_);

    // === Tab 1: Search ===
    auto* searchScroll = new QScrollArea();
    searchScroll->setWidgetResizable(true);
    searchScroll->setFrameShape(QFrame::NoFrame);

    auto* searchPage = new QWidget();
    auto* searchLayout = new QVBoxLayout(searchPage);
    searchLayout->setContentsMargins(0, 0, 0, 0);
    searchLayout->setSpacing(0);

    heroWidget_ = new HeroWidget(this);
    searchLayout->addWidget(heroWidget_);

    featureCards_ = new FeatureCards(this);
    searchLayout->addWidget(featureCards_);

    searchWidget_ = new SearchWidget(this);
    searchWidget_->setSearchHistory(searchHistory);
    searchLayout->addWidget(searchWidget_);

    // Search history quick access row
    {
        auto* historySection = new QWidget();
        historySection->setObjectName("searchHistorySection");
        auto* historyLayout = new QHBoxLayout(historySection);
        historyLayout->setContentsMargins(24, 4, 24, 4);
        historyLayout->setSpacing(6);

        auto* historyLabel = new QLabel("Recent:");
        historyLabel->setStyleSheet("color: #94a3b8; font-size: 12px; font-weight: bold;");
        historyLayout->addWidget(historyLabel);

        historyLayout->addStretch();

        auto* historyGrid = historyLayout;

        // Populate on load and after search
        connect(searchHistory, &SearchHistory::historyAdded, this, [this, historyGrid, searchHistory]() {
            // Clear old buttons
            while (historyGrid->count() > 2) {
                auto* item = historyGrid->takeAt(1);
                delete item->widget();
                delete item;
            }

            auto keywords = searchHistory->getRecentKeywords(5);
            for (const auto& kw : keywords) {
                auto* btn = new QPushButton(kw);
                btn->setStyleSheet(
                    "QPushButton { background: palette(base); border: 1px solid palette(mid); "
                    "border-radius: 12px; padding: 3px 12px; font-size: 11px; color: palette(text); }"
                    "QPushButton:hover { background: #e0e7ff; }"
                );
                btn->setCursor(Qt::PointingHandCursor);
                btn->setMaximumWidth(120);
                connect(btn, &QPushButton::clicked, this, [this, kw]() {
                    onSearch(kw);
                });
                historyGrid->insertWidget(historyGrid->count() - 1, btn);
            }
        });

        searchLayout->addWidget(historySection);
    }

    auto* resultsLayout = new QHBoxLayout();
    resultsLayout->setSpacing(12);

    resultView_ = new PaperCardView(this);
    resultView_->setFavoriteManager(favoriteManager);
    resultView_->setVisible(false);
    resultsLayout->addWidget(resultView_, 1);

    filterPanel_ = new FilterPanel(this);
    filterPanel_->setMaximumWidth(200);
    filterPanel_->setVisible(false);
    resultsLayout->addWidget(filterPanel_);

    // Filter chip bar
    filterChipBar_ = new FilterChipBar();
    filterChipBar_->setVisible(false);
    connect(filterChipBar_, &FilterChipBar::filterChanged, this,
            [this](const QMap<QString, QStringList>& active) {
        QString yearFilter = active.value("year").join(",");
        QString levelFilter = active.value("level").join(",");
        if (!currentKeyword_.isEmpty()) {
            apiManager_->searchPapers(currentKeyword_, yearFilter, levelFilter,
                                      currentOffset_, currentLimit_);
        }
    });
    searchLayout->addWidget(filterChipBar_);

    searchLayout->addLayout(resultsLayout);

    // Recent papers section
    {
        auto* recentSection = new QWidget();
        recentSection->setObjectName("recentPapersSection");
        auto* recentLayout = new QVBoxLayout(recentSection);
        recentLayout->setContentsMargins(24, 16, 24, 16);
        recentLayout->setSpacing(8);

        auto* recentHeader = new QLabel("Recent Papers");
        recentHeader->setStyleSheet("font-size: 16px; font-weight: bold; color: palette(text);");
        recentLayout->addWidget(recentHeader);

        auto* recentGrid = new QWidget();
        recentGrid->setObjectName("recentPapersGrid");
        auto* gridLayout = new QHBoxLayout(recentGrid);
        gridLayout->setObjectName("recentGridLayout");
        gridLayout->setSpacing(12);
        gridLayout->setContentsMargins(0, 0, 0, 0);
        recentLayout->addWidget(recentGrid);

        searchLayout->addWidget(recentSection);
    }

    // Add paper button
    {
        auto* addPaperBar = new QHBoxLayout();
        addPaperBar->addStretch();
        auto* addPaperBtn = new QPushButton("+ Add Paper");
        addPaperBtn->setObjectName("addPaperBtn");
        addPaperBtn->setStyleSheet(
            "QPushButton { background: #059669; color: white; border: none; border-radius: 8px; "
            "padding: 8px 20px; font-weight: bold; font-size: 12px; }"
            "QPushButton:hover { background: #047857; }"
        );
        connect(addPaperBtn, &QPushButton::clicked, this, [this]() {
            QDialog dlg(this);
            dlg.setWindowTitle("Add New Paper");
            dlg.setMinimumWidth(500);
            auto* form = new QVBoxLayout(&dlg);

            auto* titleEdit = new QLineEdit();
            titleEdit->setPlaceholderText("Title *");
            titleEdit->setStyleSheet("padding: 8px; border: 1px solid palette(mid); border-radius: 6px;");
            form->addWidget(titleEdit);

            auto* authorsEdit = new QLineEdit();
            authorsEdit->setPlaceholderText("Authors (comma-separated)");
            authorsEdit->setStyleSheet("padding: 8px; border: 1px solid palette(mid); border-radius: 6px;");
            form->addWidget(authorsEdit);

            auto* journalEdit = new QLineEdit();
            journalEdit->setPlaceholderText("Journal");
            journalEdit->setStyleSheet("padding: 8px; border: 1px solid palette(mid); border-radius: 6px;");
            form->addWidget(journalEdit);

            auto* yearEdit = new QLineEdit();
            yearEdit->setPlaceholderText("Year");
            yearEdit->setStyleSheet("padding: 8px; border: 1px solid palette(mid); border-radius: 6px;");
            form->addWidget(yearEdit);

            auto* levelCombo = new QComboBox();
            levelCombo->addItems({"", "A", "B", "C"});
            levelCombo->setStyleSheet("padding: 8px; border: 1px solid palette(mid); border-radius: 6px;");
            form->addWidget(levelCombo);

            auto* doiEdit = new QLineEdit();
            doiEdit->setPlaceholderText("DOI URL");
            doiEdit->setStyleSheet("padding: 8px; border: 1px solid palette(mid); border-radius: 6px;");
            form->addWidget(doiEdit);

            auto* abstractEdit = new QTextEdit();
            abstractEdit->setPlaceholderText("Abstract");
            abstractEdit->setMaximumHeight(100);
            abstractEdit->setStyleSheet("padding: 8px; border: 1px solid palette(mid); border-radius: 6px;");
            form->addWidget(abstractEdit);

            auto* btnRow = new QHBoxLayout();
            auto* submitBtn = new QPushButton("Submit");
            submitBtn->setStyleSheet(
                "QPushButton { background: #4f46e5; color: white; border: none; border-radius: 6px; "
                "padding: 8px 24px; font-weight: bold; }"
            );
            auto* cancelBtn = new QPushButton("Cancel");
            cancelBtn->setStyleSheet(
                "QPushButton { background: palette(button); color: palette(button-text); "
                "border: 1px solid palette(mid); border-radius: 6px; padding: 8px 24px; }"
            );
            btnRow->addStretch();
            btnRow->addWidget(cancelBtn);
            btnRow->addWidget(submitBtn);
            form->addLayout(btnRow);

            connect(cancelBtn, &QPushButton::clicked, &dlg, &QDialog::reject);
            connect(submitBtn, &QPushButton::clicked, this, [this, &dlg, titleEdit, authorsEdit,
                                                             journalEdit, yearEdit, levelCombo,
                                                             doiEdit, abstractEdit]() {
                if (titleEdit->text().trimmed().isEmpty()) {
                    titleEdit->setStyleSheet("padding: 8px; border: 2px solid #dc2626; border-radius: 6px;");
                    return;
                }
                QJsonObject data;
                data["title"] = titleEdit->text().trimmed();
                data["authors"] = authorsEdit->text().trimmed();
                data["journal"] = journalEdit->text().trimmed();
                data["year"] = yearEdit->text().trimmed();
                data["level"] = levelCombo->currentText();
                data["doi_url"] = doiEdit->text().trimmed();
                data["abstract"] = abstractEdit->toPlainText().trimmed();
                apiManager_->createPaper(data);
                statusBar()->showMessage("Paper created successfully", 5000);
                dlg.accept();
            });

            dlg.exec();
        });
        addPaperBar->addWidget(addPaperBtn);
        searchLayout->addLayout(addPaperBar);
    }

    // Advanced search section (saved searches + trending)
    {
        auto* advSection = new QWidget();
        advSection->setObjectName("advancedSearchSection");
        auto* advLayout = new QVBoxLayout(advSection);
        advLayout->setContentsMargins(24, 8, 24, 8);
        advLayout->setSpacing(8);

        // Saved searches row
        auto* savedRow = new QHBoxLayout();
        auto* savedLabel = new QLabel("Saved Searches:");
        savedLabel->setStyleSheet("font-weight: bold; color: palette(mid); font-size: 12px;");
        savedRow->addWidget(savedLabel);

        auto* savedList = new QWidget();
        savedList->setObjectName("savedSearchesList");
        auto* savedListLayout = new QHBoxLayout(savedList);
        savedListLayout->setContentsMargins(0, 0, 0, 0);
        savedListLayout->setSpacing(6);
        savedListLayout->addStretch();
        savedRow->addWidget(savedList, 1);

        auto* saveSearchBtn = new QPushButton("Save Current");
        saveSearchBtn->setObjectName("saveSearchBtn");
        saveSearchBtn->setStyleSheet(
            "QPushButton { background: #4f46e5; color: white; border: none; border-radius: 6px; "
            "padding: 4px 12px; font-size: 11px; font-weight: bold; }"
            "QPushButton:hover { background: #4338ca; }"
        );
        connect(saveSearchBtn, &QPushButton::clicked, this, [this]() {
            if (currentKeyword_.isEmpty()) {
                statusBar()->showMessage("Search first before saving", 3000);
                return;
            }
            QJsonObject data;
            data["query"] = currentKeyword_;
            data["name"] = currentKeyword_.left(30);
            apiManager_->saveSearch(data);
            statusBar()->showMessage("Search saved: " + currentKeyword_, 3000);
        });
        savedRow->addWidget(saveSearchBtn);

        auto* loadSavedBtn = new QPushButton("Load");
        loadSavedBtn->setStyleSheet(
            "QPushButton { background: palette(button); color: palette(button-text); border: 1px solid palette(mid); "
            "border-radius: 6px; padding: 4px 12px; font-size: 11px; }"
            "QPushButton:hover { background: palette(light); }"
        );
        connect(loadSavedBtn, &QPushButton::clicked, this, [this]() {
            apiManager_->getSavedSearches();
        });
        savedRow->addWidget(loadSavedBtn);

        auto* allPapersBtn = new QPushButton("All Papers");
        allPapersBtn->setStyleSheet(loadSavedBtn->styleSheet());
        connect(allPapersBtn, &QPushButton::clicked, this, [this]() {
            apiManager_->getAllPapers(1, 50);
            statusBar()->showMessage("Loading all papers...", 3000);
        });
        savedRow->addWidget(allPapersBtn);

        auto* searchHistoryBtn = new QPushButton("History");
        searchHistoryBtn->setStyleSheet(loadSavedBtn->styleSheet());
        connect(searchHistoryBtn, &QPushButton::clicked, this, [this]() {
            apiManager_->getSearchHistory();
            statusBar()->showMessage("Loading search history...", 3000);
        });
        savedRow->addWidget(searchHistoryBtn);

        advLayout->addLayout(savedRow);

        // Trending searches row
        auto* trendingRow = new QHBoxLayout();
        auto* trendingLabel = new QLabel("Trending:");
        trendingLabel->setStyleSheet("font-weight: bold; color: palette(mid); font-size: 12px;");
        trendingRow->addWidget(trendingLabel);

        auto* trendingList = new QWidget();
        trendingList->setObjectName("trendingSearchesList");
        auto* trendingListLayout = new QHBoxLayout(trendingList);
        trendingListLayout->setContentsMargins(0, 0, 0, 0);
        trendingListLayout->setSpacing(6);
        trendingListLayout->addStretch();
        trendingRow->addWidget(trendingList, 1);

        auto* refreshTrendingBtn = new QPushButton("Refresh");
        refreshTrendingBtn->setStyleSheet(loadSavedBtn->styleSheet());
        connect(refreshTrendingBtn, &QPushButton::clicked, this, [this]() {
            apiManager_->getTrendingSearches();
        });
        trendingRow->addWidget(refreshTrendingBtn);

        advLayout->addLayout(trendingRow);
        searchLayout->addWidget(advSection);
    }

    searchLayout->addStretch();

    searchScroll->setWidget(searchPage);
    tabWidget_->addTab(searchScroll, "\xf0\x9f\x94\x8d Search");

    // === Tab 2: Favorites ===
    {
        auto* page = new QWidget();
        page->setObjectName("favoritesPage");
        auto* layout = new QVBoxLayout(page);
        layout->setContentsMargins(24, 24, 24, 24);
        layout->setSpacing(12);

        auto* headerRow = new QHBoxLayout();
        auto* header = new QLabel("My Favorites");
        header->setStyleSheet("font-size: 20px; font-weight: bold; color: palette(text);");
        headerRow->addWidget(header);
        headerRow->addStretch();

        auto* countLabel = new QLabel("0 papers saved");
        countLabel->setObjectName("favCountLabel");
        countLabel->setStyleSheet("color: palette(mid); font-size: 13px;");
        headerRow->addWidget(countLabel);

        // Batch export button
        auto* exportFavBtn = new QPushButton("Export All");
        exportFavBtn->setStyleSheet(
            "QPushButton { background: #059669; color: white; border: none; "
            "border-radius: 6px; padding: 6px 16px; font-weight: bold; font-size: 12px; }"
            "QPushButton:hover { background: #047857; }"
        );
        connect(exportFavBtn, &QPushButton::clicked, this, [this, favoriteManager]() {
            auto favs = favoriteManager->getFavorites();
            if (favs.isEmpty()) {
                QMessageBox::information(this, "Export", "No favorites to export.");
                return;
            }
            QList<Paper> papers;
            for (const auto& f : favs) {
                Paper p;
                p.id = f.paperId;
                p.title = f.title;
                p.journalFull = f.journal;
                p.year = f.year;
                papers.append(p);
            }
            QString fileName = exportManager_->showSaveDialog(this, ExportFormat::CSV);
            if (!fileName.isEmpty()) {
                exportManager_->exportToCSV(fileName, papers);
            }
        });
        headerRow->addWidget(exportFavBtn);

        // Clear all button
        auto* clearFavBtn = new QPushButton("Clear All");
        clearFavBtn->setStyleSheet(
            "QPushButton { background: none; border: 1px solid #ef4444; color: #ef4444; "
            "border-radius: 6px; padding: 6px 16px; font-weight: bold; font-size: 12px; }"
            "QPushButton:hover { background: #fef2f2; }"
        );
        connect(clearFavBtn, &QPushButton::clicked, this, [this, favoriteManager]() {
            auto reply = QMessageBox::question(this, "Clear Favorites",
                "Remove all favorites?",
                QMessageBox::Yes | QMessageBox::No);
            if (reply == QMessageBox::Yes) {
                favoriteManager->clear();
                refreshFavoritesTab();
            }
        });
        headerRow->addWidget(clearFavBtn);

        layout->addLayout(headerRow);

        auto* favScroll = new QScrollArea();
        favScroll->setWidgetResizable(true);
        favScroll->setFrameShape(QFrame::NoFrame);
        auto* favContent = new QWidget();
        auto* favListLayout = new QVBoxLayout(favContent);
        favListLayout->setObjectName("favListLayout");
        favListLayout->setAlignment(Qt::AlignTop);
        favListLayout->setSpacing(8);
        favScroll->setWidget(favContent);
        layout->addWidget(favScroll, 1);

        // Refresh favorites when toggled
        auto* favMgr = favoriteManager;
        if (favMgr) {
            connect(favMgr, &FavoriteManager::favoriteAdded, this, [this]() {
                if (tabWidget_->currentIndex() == 1) refreshFavoritesTab();
            });
            connect(favMgr, &FavoriteManager::favoriteRemoved, this, [this]() {
                if (tabWidget_->currentIndex() == 1) refreshFavoritesTab();
            });
        }

        tabWidget_->addTab(page, "\xe2\xad\x90 Favorites");
    }

    // === Tab 3: Crawler ===
    {
        auto* page = new QWidget();
        auto* layout = new QVBoxLayout(page);
        layout->setContentsMargins(24, 24, 24, 24);
        layout->setSpacing(12);

        auto* headerRow = new QHBoxLayout();
        auto* header = new QLabel("Crawler Dashboard");
        header->setStyleSheet("font-size: 20px; font-weight: bold; color: palette(text);");
        headerRow->addWidget(header);
        headerRow->addStretch();

        auto* refreshBtn = new QPushButton("Refresh");
        refreshBtn->setStyleSheet(
            "QPushButton { background: #4f46e5; color: white; border: none; "
            "border-radius: 6px; padding: 6px 16px; font-weight: bold; }"
            "QPushButton:hover { background: #4338ca; }"
        );
        connect(refreshBtn, &QPushButton::clicked, this, [this]() {
            apiManager_->getCrawlerTasks();
            apiManager_->getCrawlerDashboard();
        });
        headerRow->addWidget(refreshBtn);

        // New Task button
        auto* newTaskBtn = new QPushButton("+ New Task");
        newTaskBtn->setStyleSheet(
            "QPushButton { background: #059669; color: white; border: none; "
            "border-radius: 6px; padding: 6px 16px; font-weight: bold; }"
            "QPushButton:hover { background: #047857; }"
        );
        connect(newTaskBtn, &QPushButton::clicked, this, [this]() {
            auto* dlg = new QDialog(this);
            dlg->setWindowTitle("New Crawler Task");
            dlg->setMinimumWidth(400);
            auto* form = new QVBoxLayout(dlg);

            auto* urlLabel = new QLabel("URL to crawl:");
            form->addWidget(urlLabel);
            auto* urlInput = new QLineEdit();
            urlInput->setPlaceholderText("https://dblp.org/...");
            form->addWidget(urlInput);

            auto* sourceLabel = new QLabel("Source:");
            form->addWidget(sourceLabel);
            auto* sourceCombo = new QComboBox();
            sourceCombo->addItems({"dblp", "semantic_scholar", "crossref", "arxiv"});
            form->addWidget(sourceCombo);

            auto* btnRow = new QHBoxLayout();
            auto* cancelBtn = new QPushButton("Cancel");
            connect(cancelBtn, &QPushButton::clicked, dlg, &QDialog::reject);
            btnRow->addWidget(cancelBtn);
            auto* createBtn = new QPushButton("Create");
            createBtn->setStyleSheet(
                "QPushButton { background: #4f46e5; color: white; border: none; "
                "border-radius: 6px; padding: 6px 20px; font-weight: bold; }"
            );
            connect(createBtn, &QPushButton::clicked, this, [this, dlg, urlInput, sourceCombo]() {
                QString url = urlInput->text().trimmed();
                if (url.isEmpty()) return;
                QJsonObject data;
                data["url"] = url;
                data["source"] = sourceCombo->currentText();
                apiManager_->post(apiManager_->createRequest("/api/crawler/tasks"),
                    QJsonDocument(data).toJson());
                dlg->accept();
                statusBar()->showMessage("Crawler task created for: " + url, 5000);
                // Refresh after short delay
                QTimer::singleShot(1000, this, [this]() {
                    apiManager_->getCrawlerTasks();
                    apiManager_->getCrawlerDashboard();
                });
            });
            btnRow->addWidget(createBtn);
            form->addLayout(btnRow);

            dlg->exec();
            dlg->deleteLater();
        });
        headerRow->addWidget(newTaskBtn);

        auto* crawlerStatsBtn = new QPushButton("Statistics");
        crawlerStatsBtn->setStyleSheet(
            "QPushButton { background: #0891b2; color: white; border: none; "
            "border-radius: 6px; padding: 6px 14px; font-weight: bold; font-size: 12px; }"
            "QPushButton:hover { background: #0e7490; }"
        );
        connect(crawlerStatsBtn, &QPushButton::clicked, this, [this]() {
            apiManager_->getCrawlerStatistics();
            statusBar()->showMessage("Loading crawler statistics...", 3000);
        });
        headerRow->addWidget(crawlerStatsBtn);

        auto* workersBtn = new QPushButton("Workers");
        workersBtn->setStyleSheet(
            "QPushButton { background: #059669; color: white; border: none; "
            "border-radius: 6px; padding: 6px 14px; font-weight: bold; font-size: 12px; }"
            "QPushButton:hover { background: #047857; }"
        );
        connect(workersBtn, &QPushButton::clicked, this, [this]() {
            apiManager_->getCrawlerWorkers();
            statusBar()->showMessage("Loading crawler workers...", 3000);
        });
        headerRow->addWidget(workersBtn);

        layout->addLayout(headerRow);

        // Stats row
        auto* statsRow = new QHBoxLayout();
        statsRow->setSpacing(16);
        auto makeCard = [](const QString& title, const QString& value, const QString& color) {
            auto* card = new QWidget();
            card->setStyleSheet(
                QString("QWidget { background: %1; border-radius: 8px; padding: 16px; }").arg(color)
            );
            auto* l = new QVBoxLayout(card);
            l->setContentsMargins(0, 0, 0, 0);
            l->setSpacing(4);
            auto* v = new QLabel(value);
            v->setStyleSheet("font-size: 24px; font-weight: bold; color: white;");
            v->setObjectName(title);
            l->addWidget(v);
            auto* t = new QLabel(title);
            t->setStyleSheet("font-size: 11px; color: rgba(255,255,255,0.8); font-weight: 500;");
            l->addWidget(t);
            return card;
        };
        statsRow->addWidget(makeCard("Total Tasks", "0", "#4f46e5"));
        statsRow->addWidget(makeCard("Running", "0", "#059669"));
        statsRow->addWidget(makeCard("Completed", "0", "#0891b2"));
        statsRow->addWidget(makeCard("Failed", "0", "#dc2626"));
        layout->addLayout(statsRow);

        auto* taskTable = new QTableWidget(0, 6);
        taskTable->setObjectName("crawlerTaskTable");
        taskTable->setHorizontalHeaderLabels({"ID", "Source", "URL", "Status", "Created", "Actions"});
        taskTable->horizontalHeader()->setStretchLastSection(true);
        taskTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
        taskTable->setAlternatingRowColors(true);
        layout->addWidget(taskTable, 1);

        // Templates section
        auto* tmplHeader = new QLabel("Templates");
        tmplHeader->setStyleSheet("font-size: 14px; font-weight: bold; color: palette(text); margin-top: 8px;");
        layout->addWidget(tmplHeader);

        auto* tmplRow = new QHBoxLayout();
        auto* tmplTable = new QTableWidget(0, 4);
        tmplTable->setObjectName("crawlerTemplatesTable");
        tmplTable->setHorizontalHeaderLabels({"Name", "Source", "Pattern", "Actions"});
        tmplTable->horizontalHeader()->setStretchLastSection(true);
        tmplTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
        tmplTable->setAlternatingRowColors(true);
        tmplTable->setMaximumHeight(150);
        tmplTable->setStyleSheet(
            "QTableWidget { background: palette(base); border: 1px solid palette(mid); border-radius: 8px; }"
            "QHeaderView::section { background: palette(window); color: palette(text); "
            "font-weight: bold; padding: 4px; border: none; }"
            "QTableWidget::item { padding: 2px; color: palette(text); }"
        );

        auto* newTmplBtn = new QPushButton("+ Template");
        newTmplBtn->setStyleSheet(
            "QPushButton { background: #4f46e5; color: white; border: none; border-radius: 6px; "
            "padding: 6px 14px; font-weight: bold; font-size: 11px; }"
            "QPushButton:hover { background: #4338ca; }"
        );
        connect(newTmplBtn, &QPushButton::clicked, this, [this]() {
            QDialog dlg(this);
            dlg.setWindowTitle("New Crawler Template");
            dlg.setMinimumWidth(400);
            auto* form = new QVBoxLayout(&dlg);
            auto* nameEdit = new QLineEdit();
            nameEdit->setPlaceholderText("Template name");
            nameEdit->setStyleSheet("padding: 8px; border: 1px solid palette(mid); border-radius: 6px;");
            form->addWidget(nameEdit);
            auto* sourceCombo = new QComboBox();
            sourceCombo->addItems({"ieee", "acm", "springer", "arxiv", "elsevier", "custom"});
            sourceCombo->setStyleSheet("padding: 8px; border: 1px solid palette(mid); border-radius: 6px;");
            form->addWidget(sourceCombo);
            auto* urlPatternEdit = new QLineEdit();
            urlPatternEdit->setPlaceholderText("URL pattern");
            urlPatternEdit->setStyleSheet("padding: 8px; border: 1px solid palette(mid); border-radius: 6px;");
            form->addWidget(urlPatternEdit);
            auto* btnRow = new QHBoxLayout();
            auto* okBtn = new QPushButton("Create");
            okBtn->setStyleSheet("QPushButton { background: #4f46e5; color: white; border: none; border-radius: 6px; padding: 6px 20px; font-weight: bold; }");
            auto* cancelBtn = new QPushButton("Cancel");
            cancelBtn->setStyleSheet("QPushButton { background: palette(button); color: palette(button-text); border: 1px solid palette(mid); border-radius: 6px; padding: 6px 20px; }");
            connect(cancelBtn, &QPushButton::clicked, &dlg, &QDialog::reject);
            connect(okBtn, &QPushButton::clicked, this, [this, &dlg, nameEdit, sourceCombo, urlPatternEdit]() {
                QJsonObject data;
                data["name"] = nameEdit->text().trimmed();
                data["source"] = sourceCombo->currentText();
                data["url_pattern"] = urlPatternEdit->text().trimmed();
                apiManager_->createCrawlerTemplate(data);
                dlg.accept();
                statusBar()->showMessage("Template created", 3000);
                QTimer::singleShot(500, this, [this]() { apiManager_->getCrawlerTemplates(); });
            });
            btnRow->addStretch();
            btnRow->addWidget(cancelBtn);
            btnRow->addWidget(okBtn);
            form->addLayout(btnRow);
            dlg.exec();
        });

        auto* refreshTmplBtn = new QPushButton("Refresh");
        refreshTmplBtn->setStyleSheet(
            "QPushButton { background: palette(button); color: palette(button-text); border: 1px solid palette(mid); "
            "border-radius: 6px; padding: 6px 14px; font-size: 11px; }"
            "QPushButton:hover { background: palette(light); }"
        );
        connect(refreshTmplBtn, &QPushButton::clicked, this, [this]() {
            apiManager_->getCrawlerTemplates();
        });

        tmplRow->addWidget(tmplTable, 1);
        tmplRow->addWidget(newTmplBtn);
        tmplRow->addWidget(refreshTmplBtn);
        layout->addLayout(tmplRow);

        // Schedules section
        auto* schedHeader = new QLabel("Schedules");
        schedHeader->setStyleSheet("font-size: 14px; font-weight: bold; color: palette(text); margin-top: 4px;");
        layout->addWidget(schedHeader);

        auto* schedRow = new QHBoxLayout();
        auto* schedTable = new QTableWidget(0, 4);
        schedTable->setObjectName("crawlerSchedulesTable");
        schedTable->setHorizontalHeaderLabels({"ID", "Cron", "Template", "Actions"});
        schedTable->horizontalHeader()->setStretchLastSection(true);
        schedTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
        schedTable->setAlternatingRowColors(true);
        schedTable->setMaximumHeight(120);
        schedTable->setStyleSheet(tmplTable->styleSheet());

        auto* newSchedBtn = new QPushButton("+ Schedule");
        newSchedBtn->setStyleSheet(
            "QPushButton { background: #4f46e5; color: white; border: none; border-radius: 6px; "
            "padding: 6px 14px; font-weight: bold; font-size: 11px; }"
            "QPushButton:hover { background: #4338ca; }"
        );
        connect(newSchedBtn, &QPushButton::clicked, this, [this]() {
            QDialog dlg(this);
            dlg.setWindowTitle("New Crawler Schedule");
            auto* form = new QVBoxLayout(&dlg);
            auto* cronEdit = new QLineEdit();
            cronEdit->setPlaceholderText("Cron expression (e.g. 0 */6 * * *)");
            cronEdit->setStyleSheet("padding: 8px; border: 1px solid palette(mid); border-radius: 6px;");
            form->addWidget(cronEdit);
            auto* tmplIdEdit = new QLineEdit();
            tmplIdEdit->setPlaceholderText("Template ID");
            tmplIdEdit->setStyleSheet("padding: 8px; border: 1px solid palette(mid); border-radius: 6px;");
            form->addWidget(tmplIdEdit);
            auto* btnRow = new QHBoxLayout();
            auto* okBtn = new QPushButton("Create");
            okBtn->setStyleSheet("QPushButton { background: #4f46e5; color: white; border: none; border-radius: 6px; padding: 6px 20px; font-weight: bold; }");
            auto* cancelBtn = new QPushButton("Cancel");
            cancelBtn->setStyleSheet("QPushButton { background: palette(button); color: palette(button-text); border: 1px solid palette(mid); border-radius: 6px; padding: 6px 20px; }");
            connect(cancelBtn, &QPushButton::clicked, &dlg, &QDialog::reject);
            connect(okBtn, &QPushButton::clicked, this, [this, &dlg, cronEdit, tmplIdEdit]() {
                QJsonObject data;
                data["cron"] = cronEdit->text().trimmed();
                data["template_id"] = tmplIdEdit->text().toInt();
                apiManager_->createCrawlerSchedule(data);
                dlg.accept();
                statusBar()->showMessage("Schedule created", 3000);
                QTimer::singleShot(500, this, [this]() { apiManager_->getCrawlerSchedules(); });
            });
            btnRow->addStretch();
            btnRow->addWidget(cancelBtn);
            btnRow->addWidget(okBtn);
            form->addLayout(btnRow);
            dlg.exec();
        });

        auto* refreshSchedBtn = new QPushButton("Refresh");
        refreshSchedBtn->setStyleSheet(refreshTmplBtn->styleSheet());
        connect(refreshSchedBtn, &QPushButton::clicked, this, [this]() {
            apiManager_->getCrawlerSchedules();
        });

        schedRow->addWidget(schedTable, 1);
        schedRow->addWidget(newSchedBtn);
        schedRow->addWidget(refreshSchedBtn);
        layout->addLayout(schedRow);

        // Connect templates & schedules responses
        connect(apiManager_, &ApiManager::crawlerTemplatesSuccess, this,
            [tmplTable](const QJsonArray& templates) {
                tmplTable->setRowCount(templates.size());
                for (int i = 0; i < templates.size(); ++i) {
                    auto t = templates[i].toObject();
                    tmplTable->setItem(i, 0, new QTableWidgetItem(t["name"].toString()));
                    tmplTable->setItem(i, 1, new QTableWidgetItem(t["source"].toString()));
                    tmplTable->setItem(i, 2, new QTableWidgetItem(t["url_pattern"].toString().left(40)));

                    auto* actions = new QWidget();
                    auto* al = new QHBoxLayout(actions);
                    al->setContentsMargins(4, 2, 4, 2);
                    al->setSpacing(4);
                    int tid = t["id"].toInt();

                    auto* testBtn = new QPushButton("Test");
                    testBtn->setStyleSheet("QPushButton { background: #0891b2; color: white; border: none; border-radius: 3px; padding: 2px 8px; font-size: 10px; }");
                    // testBtn would need ApiManager reference - skip for inline, use signal
                    al->addWidget(testBtn);
                    al->addStretch();
                    tmplTable->setCellWidget(i, 3, actions);
                }
            });

        connect(apiManager_, &ApiManager::crawlerTasksSuccess, this,
            [this, taskTable](const QJsonArray& tasks) {
                taskTable->setRowCount(tasks.size());
                for (int i = 0; i < tasks.size(); ++i) {
                    auto t = tasks[i].toObject();
                    taskTable->setItem(i, 0, new QTableWidgetItem(QString::number(t["id"].toInt())));
                    taskTable->setItem(i, 1, new QTableWidgetItem(t["source"].toString()));
                    taskTable->setItem(i, 2, new QTableWidgetItem(t["url"].toString().left(60)));

                    // Color-coded status
                    auto* statusItem = new QTableWidgetItem(t["status"].toString());
                    QString statusColor = t["status"].toString() == "completed" ? "#059669" :
                                          t["status"].toString() == "running" ? "#4f46e5" :
                                          t["status"].toString() == "failed" ? "#dc2626" : "#6b7280";
                    statusItem->setForeground(QColor(statusColor));
                    taskTable->setItem(i, 3, statusItem);

                    taskTable->setItem(i, 4, new QTableWidgetItem(t["created_at"].toString()));

                    // Action button
                    auto* actionWidget = new QWidget();
                    auto* actionLayout = new QHBoxLayout(actionWidget);
                    actionLayout->setContentsMargins(4, 2, 4, 2);
                    actionLayout->setSpacing(4);

                    int taskId = t["id"].toInt();
                    QString status = t["status"].toString();

                    if (status == "pending" || status == "paused") {
                        auto* startBtn = new QPushButton("Start");
                        startBtn->setStyleSheet(
                            "QPushButton { background: #059669; color: white; border: none; "
                            "border-radius: 3px; padding: 2px 8px; font-size: 10px; }"
                        );
                        connect(startBtn, &QPushButton::clicked, this, [this, taskId]() {
                            apiManager_->put(apiManager_->createRequest(
                                QString("/api/crawler/tasks/%1/start").arg(taskId)), "{}");
                        });
                        actionLayout->addWidget(startBtn);
                    }
                    if (status == "running") {
                        auto* stopBtn = new QPushButton("Stop");
                        stopBtn->setStyleSheet(
                            "QPushButton { background: #d97706; color: white; border: none; "
                            "border-radius: 3px; padding: 2px 8px; font-size: 10px; }"
                        );
                        connect(stopBtn, &QPushButton::clicked, this, [this, taskId]() {
                            apiManager_->put(apiManager_->createRequest(
                                QString("/api/crawler/tasks/%1/stop").arg(taskId)), "{}");
                        });
                        actionLayout->addWidget(stopBtn);
                    }

                    auto* delBtn = new QPushButton("Del");
                    delBtn->setStyleSheet(
                        "QPushButton { background: #dc2626; color: white; border: none; "
                        "border-radius: 3px; padding: 2px 8px; font-size: 10px; }"
                    );
                    connect(delBtn, &QPushButton::clicked, this, [this, taskId]() {
                        apiManager_->deleteResource(apiManager_->createRequest(
                            QString("/api/crawler/tasks/%1").arg(taskId)));
                        // Refresh after delete
                        QTimer::singleShot(500, this, [this]() {
                            apiManager_->getCrawlerTasks();
                            apiManager_->getCrawlerDashboard();
                        });
                    });
                    actionLayout->addWidget(delBtn);
                    actionLayout->addStretch();

                    taskTable->setCellWidget(i, 5, actionWidget);
                }
            });

        connect(apiManager_, &ApiManager::crawlerDashboardSuccess, this,
            [this, page](const QJsonObject& data) {
                auto updateCard = [page](const QString& name, const QString& value) {
                    auto* card = page->findChild<QWidget*>(name);
                    if (!card) return;
                    auto labels = card->findChildren<QLabel*>();
                    if (!labels.isEmpty()) labels[0]->setText(value);
                };
                int running = data["runningTasks"].toInt(data["running_tasks"].toInt());
                updateCard("Total Tasks", QString::number(data["totalTasks"].toInt(data["total_tasks"].toInt())));
                updateCard("Running", QString::number(running));
                updateCard("Completed", QString::number(data["completedTasks"].toInt(data["completed_tasks"].toInt())));
                updateCard("Failed", QString::number(data["failedTasks"].toInt(data["failed_tasks"].toInt())));

                // Notify when tasks complete
                if (lastRunningCount_ > 0 && running == 0) {
                    if (trayIcon_) {
                        int completed = data["completedTasks"].toInt(data["completed_tasks"].toInt());
                        trayIcon_->showMessage("Crawler Complete",
                            QString("All tasks finished. %1 completed.").arg(completed),
                            QSystemTrayIcon::Information, 3000);
                    }
                }
                lastRunningCount_ = running;
            });

        tabWidget_->addTab(page, "\xf0\x9f\x95\xb7 Crawler");
    }

    // === Tab 4: AI Assistant ===
    {
        auto* page = new QWidget();
        auto* layout = new QVBoxLayout(page);
        layout->setContentsMargins(24, 24, 24, 24);
        layout->setSpacing(12);

        auto* headerRow = new QHBoxLayout();
        auto* header = new QLabel("AI Research Assistant");
        header->setStyleSheet("font-size: 20px; font-weight: bold; color: palette(text);");
        headerRow->addWidget(header);
        headerRow->addStretch();

        // Review selected paper button
        auto* reviewBtn = new QPushButton("Review Paper");
        reviewBtn->setStyleSheet(
            "QPushButton { background: #7c3aed; color: white; border: none; "
            "border-radius: 6px; padding: 6px 16px; font-weight: bold; font-size: 12px; }"
            "QPushButton:hover { background: #6d28d9; }"
        );
        connect(reviewBtn, &QPushButton::clicked, this, [this, page]() {
            auto* chatDisplay = page->findChild<QTextEdit*>("aiChatDisplay");
            if (!chatDisplay) return;

            // Use last selected paper from resultView
            if (!resultView_ || resultView_->paperCount() == 0) {
                chatDisplay->append("<i>Select a paper first by searching and clicking a result.</i>");
                return;
            }

            // Get the first paper from current results for review
            auto papers = resultView_->getPapers();
            if (papers.isEmpty()) return;

            const auto& paper = papers.first();
            chatDisplay->append("<b>You:</b> Please review this paper: " + paper.title);

            QJsonObject data;
            data["paperId"] = paper.id;
            data["title"] = paper.title;
            data["abstract"] = paper.abstract;
            apiManager_->aiReview(data);
        });
        headerRow->addWidget(reviewBtn);

        // Export chat button
        auto* exportChatBtn = new QPushButton("Export Chat");
        exportChatBtn->setStyleSheet(
            "QPushButton { background: palette(base); color: palette(text); border: 1px solid palette(mid); "
            "border-radius: 6px; padding: 6px 12px; font-size: 12px; }"
            "QPushButton:hover { background: palette(alternate-base); }"
        );
        connect(exportChatBtn, &QPushButton::clicked, this, [this, page]() {
            auto* chatDisplay = page->findChild<QTextEdit*>("aiChatDisplay");
            if (!chatDisplay || chatDisplay->document()->isEmpty()) {
                QMessageBox::information(this, "Export", "No chat to export.");
                return;
            }
            QString fileName = QFileDialog::getSaveFileName(this,
                "Export Chat", "ai_chat.txt", "Text Files (*.txt)");
            if (fileName.isEmpty()) return;
            QFile file(fileName);
            if (file.open(QIODevice::WriteOnly)) {
                file.write(chatDisplay->toPlainText().toUtf8());
                file.close();
                statusBar()->showMessage("Chat exported to " + fileName, 3000);
            }
        });
        headerRow->addWidget(exportChatBtn);

        // AI Advanced buttons
        auto* summarizeBtn = new QPushButton("Summarize");
        summarizeBtn->setStyleSheet(
            "QPushButton { background: #0891b2; color: white; border: none; "
            "border-radius: 6px; padding: 6px 12px; font-weight: bold; font-size: 12px; }"
            "QPushButton:hover { background: #0e7490; }"
        );
        connect(summarizeBtn, &QPushButton::clicked, this, [this, page]() {
            auto* chatDisplay = page->findChild<QTextEdit*>("aiChatDisplay");
            if (!chatDisplay || !resultView_ || resultView_->paperCount() == 0) {
                if (chatDisplay) chatDisplay->append("<i>Search for papers first.</i>");
                return;
            }
            auto papers = resultView_->getPapers();
            QJsonArray paperArray;
            for (int i = 0; i < qMin(5, papers.size()); ++i) {
                QJsonObject p;
                p["id"] = papers[i].id;
                p["title"] = papers[i].title;
                paperArray.append(p);
            }
            QJsonObject data;
            data["papers"] = paperArray;
            apiManager_->aiSummarize(data);
            chatDisplay->append("<b>You:</b> Summarize the current search results");
        });
        headerRow->addWidget(summarizeBtn);

        auto* compareBtn = new QPushButton("Compare");
        compareBtn->setStyleSheet(
            "QPushButton { background: #d97706; color: white; border: none; "
            "border-radius: 6px; padding: 6px 12px; font-weight: bold; font-size: 12px; }"
            "QPushButton:hover { background: #b45309; }"
        );
        connect(compareBtn, &QPushButton::clicked, this, [this, page]() {
            auto* chatDisplay = page->findChild<QTextEdit*>("aiChatDisplay");
            if (!chatDisplay || !resultView_ || resultView_->paperCount() < 2) {
                if (chatDisplay) chatDisplay->append("<i>Need at least 2 papers to compare.</i>");
                return;
            }
            auto papers = resultView_->getPapers();
            QJsonObject data;
            data["paper1_id"] = papers[0].id;
            data["paper2_id"] = papers[1].id;
            data["paper1_title"] = papers[0].title;
            data["paper2_title"] = papers[1].title;
            apiManager_->aiCompare(data);
            chatDisplay->append("<b>You:</b> Compare the top 2 papers");
        });
        headerRow->addWidget(compareBtn);

        auto* keywordsBtn = new QPushButton("Keywords");
        keywordsBtn->setStyleSheet(
            "QPushButton { background: #059669; color: white; border: none; "
            "border-radius: 6px; padding: 6px 12px; font-weight: bold; font-size: 12px; }"
            "QPushButton:hover { background: #047857; }"
        );
        connect(keywordsBtn, &QPushButton::clicked, this, [this, page]() {
            auto* chatDisplay = page->findChild<QTextEdit*>("aiChatDisplay");
            if (!chatDisplay || !resultView_ || resultView_->paperCount() == 0) {
                if (chatDisplay) chatDisplay->append("<i>Search for papers first.</i>");
                return;
            }
            auto papers = resultView_->getPapers();
            QJsonObject data;
            data["paper_id"] = papers[0].id;
            data["title"] = papers[0].title;
            data["abstract"] = papers[0].abstract;
            apiManager_->aiKeywords(data);
            chatDisplay->append("<b>You:</b> Extract keywords from top paper");
        });
        headerRow->addWidget(keywordsBtn);

        auto* contributionsBtn = new QPushButton("Contributions");
        contributionsBtn->setStyleSheet(
            "QPushButton { background: #7c3aed; color: white; border: none; "
            "border-radius: 6px; padding: 6px 12px; font-weight: bold; font-size: 12px; }"
            "QPushButton:hover { background: #6d28d9; }"
        );
        connect(contributionsBtn, &QPushButton::clicked, this, [this, page]() {
            auto* chatDisplay = page->findChild<QTextEdit*>("aiChatDisplay");
            if (!chatDisplay || !resultView_ || resultView_->paperCount() == 0) {
                if (chatDisplay) chatDisplay->append("<i>Search for papers first.</i>");
                return;
            }
            auto papers = resultView_->getPapers();
            QJsonObject data;
            data["paper_id"] = papers[0].id;
            data["title"] = papers[0].title;
            data["abstract"] = papers[0].abstract;
            apiManager_->aiContributions(data);
            chatDisplay->append("<b>You:</b> Analyze contributions of top paper");
        });
        headerRow->addWidget(contributionsBtn);

        // Clear chat button
        auto* clearChatBtn = new QPushButton("Clear");
        clearChatBtn->setStyleSheet(
            "QPushButton { background: none; border: 1px solid #ef4444; color: #ef4444; "
            "border-radius: 6px; padding: 6px 12px; font-size: 12px; }"
            "QPushButton:hover { background: #fef2f2; }"
        );
        connect(clearChatBtn, &QPushButton::clicked, this, [page]() {
            auto* chatDisplay = page->findChild<QTextEdit*>("aiChatDisplay");
            if (chatDisplay) chatDisplay->clear();
        });
        headerRow->addWidget(clearChatBtn);

        layout->addLayout(headerRow);

        // Quick questions row
        auto* quickRow = new QHBoxLayout();
        quickRow->setSpacing(6);
        auto makeQuickBtn = [](const QString& text) {
            auto* btn = new QPushButton(text);
            btn->setStyleSheet(
                "QPushButton { background: palette(base); border: 1px solid palette(mid); "
                "border-radius: 14px; padding: 4px 14px; font-size: 11px; color: palette(text); }"
                "QPushButton:hover { background: #e0e7ff; }"
            );
            btn->setCursor(Qt::PointingHandCursor);
            return btn;
        };
        for (const auto& q : {"Summarize trends", "Compare methods", "Find gaps", "Suggest keywords"}) {
            auto* btn = makeQuickBtn(q);
            connect(btn, &QPushButton::clicked, this, [this, btn]() {
                // Forward to chat input
                auto* page = btn->parentWidget();
                while (page && !page->inherits("QWidget")) page = page->parentWidget();
                auto* chatInput = page ? page->findChild<QLineEdit*>() : nullptr;
                auto* chatDisplay = page ? page->findChild<QTextEdit*>("aiChatDisplay") : nullptr;
                if (chatInput && chatDisplay) {
                    chatDisplay->append("<b>You:</b> " + btn->text());
                    QJsonObject data;
                    data["message"] = btn->text();
                    data["userId"] = 1;
                    apiManager_->aiChat(data);
                }
            });
            quickRow->addWidget(btn);
        }
        layout->addLayout(quickRow);

        // AI Copilot section
        {
            auto* copilotBar = new QHBoxLayout();
            copilotBar->setSpacing(8);

            auto* copilotLabel = new QLabel("AI Copilot:");
            copilotLabel->setStyleSheet("font-weight: bold; color: palette(mid); font-size: 12px;");
            copilotBar->addWidget(copilotLabel);

            auto* litReviewBtn = new QPushButton("Literature Review");
            litReviewBtn->setStyleSheet(
                "QPushButton { background: #7c3aed; color: white; border: none; border-radius: 6px; "
                "padding: 4px 12px; font-size: 11px; font-weight: bold; }"
                "QPushButton:hover { background: #6d28d9; }"
            );
            connect(litReviewBtn, &QPushButton::clicked, this, [this]() {
                QDialog dlg(this);
                dlg.setWindowTitle("Generate Literature Review");
                dlg.setMinimumWidth(500);
                auto* form = new QVBoxLayout(&dlg);
                auto* topicEdit = new QLineEdit();
                topicEdit->setPlaceholderText("Research topic (e.g., transformer architectures)");
                topicEdit->setStyleSheet("padding: 8px; border: 1px solid palette(mid); border-radius: 6px;");
                form->addWidget(topicEdit);
                auto* focusEdit = new QTextEdit();
                focusEdit->setPlaceholderText("Focus areas (optional, one per line)");
                focusEdit->setMaximumHeight(80);
                focusEdit->setStyleSheet("padding: 8px; border: 1px solid palette(mid); border-radius: 6px;");
                form->addWidget(focusEdit);
                auto* btnRow = new QHBoxLayout();
                auto* genBtn = new QPushButton("Generate");
                genBtn->setStyleSheet("QPushButton { background: #7c3aed; color: white; border: none; border-radius: 6px; padding: 8px 24px; font-weight: bold; }");
                auto* cancelBtn = new QPushButton("Cancel");
                cancelBtn->setStyleSheet("QPushButton { background: palette(button); color: palette(button-text); border: 1px solid palette(mid); border-radius: 6px; padding: 8px 20px; }");
                connect(cancelBtn, &QPushButton::clicked, &dlg, &QDialog::reject);
                connect(genBtn, &QPushButton::clicked, this, [this, &dlg, topicEdit, focusEdit]() {
                    if (topicEdit->text().trimmed().isEmpty()) return;
                    QJsonObject data;
                    data["topic"] = topicEdit->text().trimmed();
                    data["focus_areas"] = focusEdit->toPlainText();
                    apiManager_->generateLiteratureReview(data);
                    dlg.accept();
                    statusBar()->showMessage("Generating literature review...", 5000);
                });
                btnRow->addStretch();
                btnRow->addWidget(cancelBtn);
                btnRow->addWidget(genBtn);
                form->addLayout(btnRow);
                dlg.exec();
            });
            copilotBar->addWidget(litReviewBtn);

            auto* researchPlanBtn = new QPushButton("Research Plan");
            researchPlanBtn->setStyleSheet(
                "QPushButton { background: #0891b2; color: white; border: none; border-radius: 6px; "
                "padding: 4px 12px; font-size: 11px; font-weight: bold; }"
                "QPushButton:hover { background: #0e7490; }"
            );
            connect(researchPlanBtn, &QPushButton::clicked, this, [this]() {
                QDialog dlg(this);
                dlg.setWindowTitle("Generate Research Plan");
                dlg.setMinimumWidth(500);
                auto* form = new QVBoxLayout(&dlg);
                auto* topicEdit = new QLineEdit();
                topicEdit->setPlaceholderText("Research topic");
                topicEdit->setStyleSheet("padding: 8px; border: 1px solid palette(mid); border-radius: 6px;");
                form->addWidget(topicEdit);
                auto* goalEdit = new QTextEdit();
                goalEdit->setPlaceholderText("Research goals (one per line)");
                goalEdit->setMaximumHeight(80);
                goalEdit->setStyleSheet("padding: 8px; border: 1px solid palette(mid); border-radius: 6px;");
                form->addWidget(goalEdit);
                auto* btnRow = new QHBoxLayout();
                auto* genBtn = new QPushButton("Generate");
                genBtn->setStyleSheet("QPushButton { background: #0891b2; color: white; border: none; border-radius: 6px; padding: 8px 24px; font-weight: bold; }");
                auto* cancelBtn = new QPushButton("Cancel");
                cancelBtn->setStyleSheet("QPushButton { background: palette(button); color: palette(button-text); border: 1px solid palette(mid); border-radius: 6px; padding: 8px 20px; }");
                connect(cancelBtn, &QPushButton::clicked, &dlg, &QDialog::reject);
                connect(genBtn, &QPushButton::clicked, this, [this, &dlg, topicEdit, goalEdit]() {
                    if (topicEdit->text().trimmed().isEmpty()) return;
                    QJsonObject data;
                    data["topic"] = topicEdit->text().trimmed();
                    data["goals"] = goalEdit->toPlainText();
                    apiManager_->generateResearchPlan(data);
                    dlg.accept();
                    statusBar()->showMessage("Generating research plan...", 5000);
                });
                btnRow->addStretch();
                btnRow->addWidget(cancelBtn);
                btnRow->addWidget(genBtn);
                form->addLayout(btnRow);
                dlg.exec();
            });
            copilotBar->addWidget(researchPlanBtn);

            auto* myReviewsBtn = new QPushButton("My Reviews");
            myReviewsBtn->setStyleSheet(
                "QPushButton { background: palette(button); color: palette(button-text); border: 1px solid palette(mid); "
                "border-radius: 6px; padding: 4px 12px; font-size: 11px; }"
                "QPushButton:hover { background: palette(light); }"
            );
            connect(myReviewsBtn, &QPushButton::clicked, this, [this]() {
                apiManager_->getLiteratureReviews();
                apiManager_->getResearchPlans();
                statusBar()->showMessage("Loading your reviews and plans...", 3000);
            });
            copilotBar->addWidget(myReviewsBtn);

            auto* aiStatusBtn = new QPushButton("AI Status");
            aiStatusBtn->setStyleSheet(myReviewsBtn->styleSheet());
            connect(aiStatusBtn, &QPushButton::clicked, this, [this]() {
                apiManager_->getAiStatus();
            });
            copilotBar->addWidget(aiStatusBtn);

            copilotBar->addStretch();
            layout->addLayout(copilotBar);
        }

        auto* chatDisplay = new QTextEdit();
        chatDisplay->setObjectName("aiChatDisplay");
        chatDisplay->setReadOnly(true);
        chatDisplay->setStyleSheet(
            "QTextEdit { background: palette(base); border: 1px solid palette(mid); "
            "border-radius: 8px; padding: 12px; font-size: 14px; }"
        );
        chatDisplay->setPlaceholderText("AI responses will appear here...");
        layout->addWidget(chatDisplay, 1);

        auto* inputRow = new QHBoxLayout();
        auto* chatInput = new QLineEdit();
        chatInput->setPlaceholderText("Ask about papers, research topics...");
        chatInput->setStyleSheet("QLineEdit { padding: 10px; border: 1px solid palette(mid); border-radius: 8px; }");
        inputRow->addWidget(chatInput, 1);

        auto* sendBtn = new QPushButton("Send");
        sendBtn->setStyleSheet(
            "QPushButton { background: #4f46e5; color: white; border: none; "
            "border-radius: 8px; padding: 10px 24px; font-weight: bold; }"
            "QPushButton:hover { background: #4338ca; }"
        );
        inputRow->addWidget(sendBtn);
        layout->addLayout(inputRow);

        connect(sendBtn, &QPushButton::clicked, this, [this, chatInput, chatDisplay]() {
            QString msg = chatInput->text().trimmed();
            if (msg.isEmpty()) return;
            chatDisplay->append("<b>You:</b> " + msg);
            chatInput->clear();
            QJsonObject data;
            data["message"] = msg;
            data["userId"] = 1;
            apiManager_->aiChat(data);
        });
        connect(chatInput, &QLineEdit::returnPressed, sendBtn, &QPushButton::click);

        connect(apiManager_, &ApiManager::aiChatSuccess, this, [chatDisplay](const QString& response) {
            chatDisplay->append("<b>AI:</b> " + response);
        });

        connect(apiManager_, &ApiManager::aiReviewSuccess, this, [chatDisplay](const QJsonObject& result) {
            QString review = result["review"].toString(result["summary"].toString("Review completed."));
            chatDisplay->append("<b>AI Review:</b> " + review);
        });

        tabWidget_->addTab(page, "\xf0\x9f\xa4\x96 AI");
    }

    // === Tab 5: Statistics ===
    {
        auto* page = new QWidget();
        auto* layout = new QVBoxLayout(page);
        layout->setContentsMargins(24, 24, 24, 24);
        layout->setSpacing(16);

        auto* header = new QLabel("Statistics");
        header->setStyleSheet("font-size: 20px; font-weight: bold; color: palette(text);");
        layout->addWidget(header);

        // Summary cards row
        auto* cardsRow = new QHBoxLayout();
        cardsRow->setSpacing(12);

        auto makeStatCard = [](const QString& name, const QString& icon, const QString& color) {
            auto* card = new QWidget();
            card->setObjectName(name);
            card->setStyleSheet(
                QString("QWidget { background: %1; border-radius: 8px; padding: 16px; }").arg(color)
            );
            auto* l = new QVBoxLayout(card);
            l->setContentsMargins(0, 0, 0, 0);
            l->setSpacing(4);
            auto* iconLabel = new QLabel(icon);
            iconLabel->setStyleSheet("font-size: 20px;");
            l->addWidget(iconLabel);
            auto* v = new QLabel("--");
            v->setObjectName(name + "Value");
            v->setStyleSheet("font-size: 24px; font-weight: bold; color: white;");
            l->addWidget(v);
            auto* t = new QLabel(name);
            t->setStyleSheet("font-size: 11px; color: rgba(255,255,255,0.8); font-weight: 500;");
            l->addWidget(t);
            return card;
        };

        cardsRow->addWidget(makeStatCard("Papers", "📄", "#4f46e5"));
        cardsRow->addWidget(makeStatCard("Journals", "📚", "#059669"));
        cardsRow->addWidget(makeStatCard("Authors", "👥", "#0891b2"));
        cardsRow->addWidget(makeStatCard("Crawled", "🕷️", "#d97706"));
        layout->addLayout(cardsRow);

        // Year distribution table
        auto* yearHeader = new QLabel("Year Distribution");
        yearHeader->setStyleSheet("font-size: 16px; font-weight: bold; color: palette(text);");
        layout->addWidget(yearHeader);

        auto* yearTable = new QTableWidget(0, 3);
        yearTable->setObjectName("statsYearTable");
        yearTable->setHorizontalHeaderLabels({"Year", "Count", "Percentage"});
        yearTable->horizontalHeader()->setStretchLastSection(true);
        yearTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
        yearTable->setAlternatingRowColors(true);
        yearTable->setMaximumHeight(200);
        layout->addWidget(yearTable);

        // Top journals table
        auto* journalHeader = new QLabel("Top Journals");
        journalHeader->setStyleSheet("font-size: 16px; font-weight: bold; color: palette(text);");
        layout->addWidget(journalHeader);

        auto* journalTable = new QTableWidget(0, 3);
        journalTable->setObjectName("statsJournalTable");
        journalTable->setHorizontalHeaderLabels({"Journal", "Papers", "Level"});
        journalTable->horizontalHeader()->setStretchLastSection(true);
        journalTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
        journalTable->setAlternatingRowColors(true);
        layout->addWidget(journalTable, 1);

        // Detailed stats buttons
        auto* detailStatsBar = new QHBoxLayout();
        auto* sysStatsBtn = new QPushButton("System Stats");
        sysStatsBtn->setStyleSheet(
            "QPushButton { background: #4f46e5; color: white; border: none; border-radius: 6px; "
            "padding: 6px 14px; font-weight: bold; font-size: 11px; }"
            "QPushButton:hover { background: #4338ca; }"
        );
        connect(sysStatsBtn, &QPushButton::clicked, this, [this]() {
            apiManager_->getSystemStats();
        });
        auto* resStatsBtn = new QPushButton("Resource Stats");
        resStatsBtn->setStyleSheet(
            "QPushButton { background: #0891b2; color: white; border: none; border-radius: 6px; "
            "padding: 6px 14px; font-weight: bold; font-size: 11px; }"
            "QPushButton:hover { background: #0e7490; }"
        );
        connect(resStatsBtn, &QPushButton::clicked, this, [this]() {
            apiManager_->getResourceStats();
        });
        auto* perfStatsBtn = new QPushButton("Performance Stats");
        perfStatsBtn->setStyleSheet(
            "QPushButton { background: #059669; color: white; border: none; border-radius: 6px; "
            "padding: 6px 14px; font-weight: bold; font-size: 11px; }"
            "QPushButton:hover { background: #047857; }"
        );
        connect(perfStatsBtn, &QPushButton::clicked, this, [this]() {
            apiManager_->getPerformanceStats();
        });
        detailStatsBar->addWidget(sysStatsBtn);
        detailStatsBar->addWidget(resStatsBtn);
        detailStatsBar->addWidget(perfStatsBtn);
        detailStatsBar->addStretch();
        layout->addLayout(detailStatsBar);

        connect(apiManager_, &ApiManager::statsSuccess, this,
            [page](const QJsonObject& stats) {
                // Summary cards
                int totalPapers = stats["total_papers"].toInt(stats["totalPapers"].toInt(0));
                int totalJournals = stats["total_journals"].toInt(stats["totalJournals"].toInt(0));
                int totalAuthors = stats["total_authors"].toInt(stats["totalAuthors"].toInt(0));
                int crawled = stats["crawled_count"].toInt(stats["crawledCount"].toInt(0));

                auto updateVal = [page](const QString& name, const QString& val) {
                    auto* label = page->findChild<QLabel*>(name + "Value");
                    if (label) label->setText(val);
                };
                updateVal("Papers", QString::number(totalPapers));
                updateVal("Journals", QString::number(totalJournals));
                updateVal("Authors", QString::number(totalAuthors));
                updateVal("Crawled", QString::number(crawled));

                // Year distribution
                auto* yearTable = page->findChild<QTableWidget*>("statsYearTable");
                if (yearTable) {
                    QJsonObject yearDist = stats["year_distribution"].toObject(stats["yearDistribution"].toObject());
                    yearTable->setRowCount(yearDist.size());
                    int row = 0;
                    for (auto it = yearDist.begin(); it != yearDist.end(); ++it, ++row) {
                        yearTable->setItem(row, 0, new QTableWidgetItem(it.key()));
                        yearTable->setItem(row, 1, new QTableWidgetItem(QString::number(it.value().toInt())));
                        double pct = totalPapers > 0 ? (it.value().toInt() * 100.0 / totalPapers) : 0;
                        yearTable->setItem(row, 2, new QTableWidgetItem(QString("%1%").arg(pct, 0, 'f', 1)));
                    }
                }

                // Top journals
                auto* journalTable = page->findChild<QTableWidget*>("statsJournalTable");
                if (journalTable) {
                    QJsonArray topJournals = stats["top_journals"].toArray(stats["topJournals"].toArray());
                    journalTable->setRowCount(topJournals.size());
                    for (int i = 0; i < topJournals.size(); ++i) {
                        auto j = topJournals[i].toObject();
                        journalTable->setItem(i, 0, new QTableWidgetItem(j["name"].toString(j["journal"].toString())));
                        journalTable->setItem(i, 1, new QTableWidgetItem(QString::number(j["count"].toInt(j["paper_count"].toInt()))));
                        journalTable->setItem(i, 2, new QTableWidgetItem(j["level"].toString()));
                    }
                }
            });

        tabWidget_->addTab(page, "\xf0\x9f\x93\x8a Statistics");
    }

    // === Tab 6: Recommendations ===
    {
        auto* page = new QWidget();
        auto* layout = new QVBoxLayout(page);
        layout->setContentsMargins(20, 16, 20, 16);
        layout->setSpacing(16);

        // Header
        auto* header = new QLabel("\xf0\x9f\x92\xa1 Recommendations");
        header->setStyleSheet("font-size: 20px; font-weight: bold; color: palette(text);");
        layout->addWidget(header);

        // Recommendations list with scroll
        auto* scroll = new QScrollArea();
        scroll->setWidgetResizable(true);
        scroll->setStyleSheet("QScrollArea { border: none; background: transparent; }");
        auto* recContainer = new QWidget();
        recContainer->setObjectName("recContainer");
        auto* recLayout = new QVBoxLayout(recContainer);
        recLayout->setSpacing(12);
        recLayout->setObjectName("recListLayout");

        auto* recLoading = new QLabel("Loading recommendations...");
        recLoading->setObjectName("recLoadingLabel");
        recLoading->setAlignment(Qt::AlignCenter);
        recLoading->setStyleSheet("color: palette(mid); font-size: 13px; padding: 40px;");
        recLayout->addWidget(recLoading);
        recLayout->addStretch();

        scroll->setWidget(recContainer);
        layout->addWidget(scroll, 1);

        // Bottom bar: trending + refresh
        auto* bottomBar = new QHBoxLayout();
        auto* trendingBtn = new QPushButton("\xf0\x9f\x94\xa5 Trending Papers");
        trendingBtn->setObjectName("trendingBtn");
        trendingBtn->setStyleSheet(
            "QPushButton { background: #f59e0b; color: white; border: none; border-radius: 8px; "
            "padding: 8px 20px; font-weight: bold; font-size: 12px; }"
            "QPushButton:hover { background: #d97706; }"
        );
        auto* refreshRecBtn = new QPushButton("\xf0\x9f\x94\x84 Refresh");
        refreshRecBtn->setObjectName("refreshRecBtn");
        refreshRecBtn->setStyleSheet(
            "QPushButton { background: palette(button); color: palette(button-text); border: 1px solid palette(mid); "
            "border-radius: 8px; padding: 8px 20px; font-weight: bold; font-size: 12px; }"
            "QPushButton:hover { background: palette(light); }"
        );
        bottomBar->addWidget(trendingBtn);
        bottomBar->addStretch();
        bottomBar->addWidget(refreshRecBtn);
        layout->addLayout(bottomBar);

        // Signals
        connect(trendingBtn, &QPushButton::clicked, this, [this]() {
            apiManager_->getTrendingPapers();
            statusBar()->showMessage("Loading trending papers...", 3000);
        });
        connect(refreshRecBtn, &QPushButton::clicked, this, [this]() {
            apiManager_->getPaperRecommendations();
            statusBar()->showMessage("Refreshing recommendations...", 3000);
        });

        tabWidget_->addTab(page, "\xf0\x9f\x92\xa1 Recommend");
    }

    // === Tab 7: Admin ===
    {
        auto* page = new QWidget();
        auto* layout = new QVBoxLayout(page);
        layout->setContentsMargins(20, 16, 20, 16);
        layout->setSpacing(16);

        // Header
        auto* header = new QLabel("\xf0\x9f\x94\xa7 Admin Panel");
        header->setStyleSheet("font-size: 20px; font-weight: bold; color: palette(text);");
        layout->addWidget(header);

        // Summary cards row
        auto* cardsRow = new QHBoxLayout();
        cardsRow->setSpacing(12);

        auto makeAdminCard = [](const QString& name, const QString& icon, const QString& color) {
            auto* card = new QWidget();
            card->setObjectName(name);
            card->setStyleSheet(
                QString("QWidget { background: %1; border-radius: 8px; padding: 16px; }").arg(color)
            );
            auto* l = new QVBoxLayout(card);
            l->setContentsMargins(0, 0, 0, 0);
            l->setSpacing(4);
            auto* iconLabel = new QLabel(icon);
            iconLabel->setStyleSheet("font-size: 20px;");
            l->addWidget(iconLabel);
            auto* v = new QLabel("--");
            v->setObjectName(name + "Value");
            v->setStyleSheet("font-size: 24px; font-weight: bold; color: white;");
            l->addWidget(v);
            auto* t = new QLabel(name);
            t->setStyleSheet("font-size: 11px; color: rgba(255,255,255,0.8); font-weight: 500;");
            l->addWidget(t);
            return card;
        };
        cardsRow->addWidget(makeAdminCard("Users", "\xf0\x9f\x91\xa4", "#6366f1"));
        cardsRow->addWidget(makeAdminCard("Modules", "\xf0\x9f\xa7\xa9", "#0891b2"));
        cardsRow->addWidget(makeAdminCard("Uptime", "\xe2\x8f\xb1\xef\xb8\x8f", "#059669"));
        cardsRow->addWidget(makeAdminCard("Requests", "\xf0\x9f\x93\xa1", "#d97706"));
        layout->addLayout(cardsRow);

        // Users table
        auto* usersHeader = new QLabel("Users");
        usersHeader->setStyleSheet("font-size: 16px; font-weight: bold; color: palette(text);");
        layout->addWidget(usersHeader);

        auto* usersTable = new QTableWidget(0, 5);
        usersTable->setObjectName("adminUsersTable");
        usersTable->setHorizontalHeaderLabels({"ID", "Username", "Email", "Role", "Status"});
        usersTable->horizontalHeader()->setStretchLastSection(true);
        usersTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
        usersTable->setAlternatingRowColors(true);
        usersTable->setMaximumHeight(250);
        usersTable->setStyleSheet(
            "QTableWidget { background: palette(base); border: 1px solid palette(mid); border-radius: 8px; }"
            "QHeaderView::section { background: palette(window); color: palette(text); "
            "font-weight: bold; padding: 6px; border: none; border-bottom: 1px solid palette(mid); }"
            "QTableWidget::item { padding: 4px; color: palette(text); }"
            "QTableWidget::item:alternate { background: palette(alternate-base); }"
        );
        layout->addWidget(usersTable);

        // Modules section
        auto* modulesHeader = new QLabel("Modules");
        modulesHeader->setStyleSheet("font-size: 16px; font-weight: bold; color: palette(text);");
        layout->addWidget(modulesHeader);

        auto* modulesTable = new QTableWidget(0, 4);
        modulesTable->setObjectName("adminModulesTable");
        modulesTable->setHorizontalHeaderLabels({"Name", "Version", "Status", "Action"});
        modulesTable->horizontalHeader()->setStretchLastSection(true);
        modulesTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
        modulesTable->setAlternatingRowColors(true);
        modulesTable->setMaximumHeight(200);
        modulesTable->setStyleSheet(usersTable->styleSheet());
        layout->addWidget(modulesTable);

        // System monitor section
        auto* monitorHeader = new QLabel("System Monitor");
        monitorHeader->setStyleSheet("font-size: 16px; font-weight: bold; color: palette(text);");
        layout->addWidget(monitorHeader);

        auto* monitorInfo = new QLabel("CPU: -- | Memory: -- | Disk: --");
        monitorInfo->setObjectName("adminMonitorInfo");
        monitorInfo->setStyleSheet(
            "padding: 12px; background: palette(base); border: 1px solid palette(mid); "
            "border-radius: 8px; color: palette(text); font-size: 13px;"
        );
        layout->addWidget(monitorInfo);

        // Bottom actions
        auto* adminActions = new QHBoxLayout();
        auto* refreshAdminBtn = new QPushButton("\xf0\x9f\x94\x84 Refresh All");
        refreshAdminBtn->setObjectName("refreshAdminBtn");
        refreshAdminBtn->setStyleSheet(
            "QPushButton { background: #6366f1; color: white; border: none; border-radius: 8px; "
            "padding: 8px 20px; font-weight: bold; font-size: 12px; }"
            "QPushButton:hover { background: #4f46e5; }"
        );
        auto* loginHistoryBtn = new QPushButton("\xf0\x9f\x93\x8b Login History");
        loginHistoryBtn->setObjectName("loginHistoryBtn");
        loginHistoryBtn->setStyleSheet(
            "QPushButton { background: palette(button); color: palette(button-text); border: 1px solid palette(mid); "
            "border-radius: 8px; padding: 8px 20px; font-weight: bold; font-size: 12px; }"
            "QPushButton:hover { background: palette(light); }"
        );
        auto* perfMetricsBtn = new QPushButton("\xf0\x9f\x93\x8a Performance");
        perfMetricsBtn->setObjectName("perfMetricsBtn");
        perfMetricsBtn->setStyleSheet(loginHistoryBtn->styleSheet());
        adminActions->addWidget(refreshAdminBtn);
        adminActions->addStretch();
        adminActions->addWidget(loginHistoryBtn);
        adminActions->addWidget(perfMetricsBtn);
        layout->addLayout(adminActions);

        // Admin button signals
        connect(refreshAdminBtn, &QPushButton::clicked, this, [this]() {
            apiManager_->getAdminDashboard();
            apiManager_->getAdminUsers();
            apiManager_->getAdminModules();
            apiManager_->getSystemMonitor();
            statusBar()->showMessage("Refreshing admin data...", 3000);
        });
        connect(loginHistoryBtn, &QPushButton::clicked, this, [this]() {
            apiManager_->getLoginHistory();
            statusBar()->showMessage("Loading login history...", 3000);
        });
        connect(perfMetricsBtn, &QPushButton::clicked, this, [this]() {
            apiManager_->getPerformanceMetrics();
            statusBar()->showMessage("Loading performance metrics...", 3000);
        });

        tabWidget_->addTab(page, "\xf0\x9f\x94\xa7 Admin");
    }

    // === Tab 8: LaTeX Editor ===
    {
        auto* page = new QWidget();
        page->setObjectName("latexEditorPage");
        auto* layout = new QVBoxLayout(page);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(0);

        latexEditor_ = new LatexEditorWidget(apiManager_, this);
        layout->addWidget(latexEditor_);

        connect(latexEditor_, &LatexEditorWidget::statusMessage, this, [this](const QString& msg) {
            statusBar()->showMessage(msg, 3000);
        });

        tabWidget_->addTab(page, "\xf0\x9f\x93\x9d LaTeX");
    }

    // Update tab position to bottom with scroll buttons for 8 tabs
    tabWidget_->setUsesScrollButtons(true);
}

void MainWindow::paintEvent(QPaintEvent* event) {
    Q_UNUSED(event);

    // Draw gradient background
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // Create gradient matching web frontend
    QLinearGradient gradient(rect().topLeft(), rect().bottomRight());
    auto colors = themeManager_->colors();

    gradient.setColorAt(0, colors.backgroundStart);
    gradient.setColorAt(1, colors.backgroundEnd);

    painter.fillRect(rect(), gradient);
}

void MainWindow::resizeEvent(QResizeEvent* event) {
    QMainWindow::resizeEvent(event);
    if (themeButton_) {
        themeButton_->move(width() - 70, 80);
    }
}

void MainWindow::populateRecommendations(const QJsonObject& data) {
    auto* container = findChild<QWidget*>("recContainer");
    if (!container) return;
    auto* layout = container->findChild<QVBoxLayout*>("recListLayout");
    if (!layout) return;

    // Clear old content
    QLayoutItem* item;
    while ((item = layout->takeAt(0)) != nullptr) {
        delete item->widget();
        delete item;
    }

    QJsonArray papers = data["papers"].toArray(data["recommendations"].toArray());
    QString source = data["source"].toString("recommendations");

    if (papers.isEmpty()) {
        auto* emptyLabel = new QLabel("No recommendations available yet. Try searching for papers first!");
        emptyLabel->setAlignment(Qt::AlignCenter);
        emptyLabel->setStyleSheet("color: palette(mid); font-size: 14px; padding: 60px;");
        layout->addWidget(emptyLabel);
        layout->addStretch();
        return;
    }

    // Source header
    QString sourceText = source == "trending" ? "\xf0\x9f\x94\xa5 Trending Papers"
                       : source == "similar" ? "\xf0\x9f\x94\x97 Similar Papers"
                       : "\xf0\x9f\x92\xa1 Recommended for You";
    auto* sourceLabel = new QLabel(sourceText);
    sourceLabel->setStyleSheet("font-size: 16px; font-weight: bold; color: palette(text); padding: 8px 0;");
    layout->addWidget(sourceLabel);

    for (const auto& val : papers) {
        auto p = val.toObject();
        auto* card = new QWidget();
        card->setStyleSheet(
            "QWidget { background: palette(base); border: 1px solid palette(mid); border-radius: 10px; }"
        );
        card->setCursor(Qt::PointingHandCursor);

        auto* cardLayout = new QVBoxLayout(card);
        cardLayout->setContentsMargins(16, 12, 16, 12);
        cardLayout->setSpacing(6);

        // Title
        QString title = p["title"].toString(p["paper_title"].toString());
        auto* titleLabel = new QLabel(title.left(120));
        titleLabel->setWordWrap(true);
        titleLabel->setStyleSheet("font-weight: bold; font-size: 13px; color: palette(text);");
        cardLayout->addWidget(titleLabel);

        // Meta row
        auto* metaLayout = new QHBoxLayout();
        QString journal = p["journal"].toString(p["journal_name"].toString());
        QString year = p["year"].toString(p["publication_year"].toString());
        QString score = p["score"].toString(p["relevance_score"].toString());

        auto* metaLabel = new QLabel(journal + (year.isEmpty() ? "" : " | " + year));
        metaLabel->setStyleSheet("color: palette(mid); font-size: 11px;");
        metaLayout->addWidget(metaLabel);
        metaLayout->addStretch();

        if (!score.isEmpty()) {
            auto* scoreLabel = new QLabel("\xe2\xad\x90 " + score.left(5));
            scoreLabel->setStyleSheet("color: #f59e0b; font-weight: bold; font-size: 11px;");
            metaLayout->addWidget(scoreLabel);
        }
        cardLayout->addLayout(metaLayout);

        // Authors
        QString authors = p["authors"].toString();
        if (!authors.isEmpty()) {
            auto* authorLabel = new QLabel(authors.left(100));
            authorLabel->setStyleSheet("color: palette(mid); font-size: 10px;");
            cardLayout->addWidget(authorLabel);
        }

        // Action buttons
        auto* actions = new QHBoxLayout();
        int paperId = p["id"].toInt(p["paper_id"].toInt(0));

        auto* viewBtn = new QPushButton("View Details");
        viewBtn->setStyleSheet(
            "QPushButton { background: #4f46e5; color: white; border: none; border-radius: 6px; "
            "padding: 4px 12px; font-size: 11px; font-weight: bold; }"
            "QPushButton:hover { background: #4338ca; }"
        );
        connect(viewBtn, &QPushButton::clicked, this, [this, paperId]() {
            onPaperSelected(paperId);
        });
        actions->addWidget(viewBtn);

        auto* similarBtn = new QPushButton("\xf0\x9f\x94\x97 Similar");
        similarBtn->setStyleSheet(
            "QPushButton { background: palette(button); color: palette(button-text); border: 1px solid palette(mid); "
            "border-radius: 6px; padding: 4px 12px; font-size: 11px; }"
            "QPushButton:hover { background: palette(light); }"
        );
        connect(similarBtn, &QPushButton::clicked, this, [this, paperId]() {
            apiManager_->getSimilarPapers(paperId);
            statusBar()->showMessage("Finding similar papers...", 3000);
        });
        actions->addWidget(similarBtn);

        auto* feedbackBtn = new QPushButton("\xf0\x9f\x91\x8d");
        feedbackBtn->setStyleSheet(
            "QPushButton { background: transparent; border: 1px solid palette(mid); border-radius: 6px; "
            "padding: 4px 8px; font-size: 12px; }"
            "QPushButton:hover { background: #dcfce7; }"
        );
        connect(feedbackBtn, &QPushButton::clicked, this, [this, paperId]() {
            QJsonObject feedback;
            feedback["paper_id"] = paperId;
            feedback["action"] = "positive";
            apiManager_->submitRecommendationFeedback(feedback);
        });
        actions->addWidget(feedbackBtn);

        auto* whyBtn = new QPushButton("Why?");
        whyBtn->setStyleSheet(
            "QPushButton { background: transparent; border: 1px solid palette(mid); border-radius: 6px; "
            "padding: 4px 8px; font-size: 11px; color: palette(text); }"
            "QPushButton:hover { background: palette(alternate-base); }"
        );
        connect(whyBtn, &QPushButton::clicked, this, [this, paperId]() {
            apiManager_->getRecommendationExplanation(paperId);
        });
        actions->addWidget(whyBtn);

        actions->addStretch();
        cardLayout->addLayout(actions);

        // Click on card to view details
        connect(card, &QWidget::mousePressEvent, this, [this, paperId](QMouseEvent*) {
            onPaperSelected(paperId);
        });

        layout->addWidget(card);
    }

    layout->addStretch();
    int total = data["total"].toInt(papers.size());
    statusBar()->showMessage(QString("Loaded %1 papers").arg(total), 3000);
}

void MainWindow::populateAdminDashboard(const QJsonObject& data) {
    auto updateVal = [this](const QString& name, const QString& val) {
        auto* label = findChild<QLabel*>(name + "Value");
        if (label) label->setText(val);
    };
    updateVal("Users", QString::number(data["total_users"].toInt(data["user_count"].toInt(0))));
    updateVal("Modules", QString::number(data["active_modules"].toInt(data["module_count"].toInt(0))));
    updateVal("Uptime", data["uptime"].toString(data["uptime_human"].toString("--")));
    updateVal("Requests", QString::number(data["total_requests"].toInt(data["request_count"].toInt(0))));
}

void MainWindow::populateAdminUsers(const QJsonObject& data) {
    auto* table = findChild<QTableWidget*>("adminUsersTable");
    if (!table) return;

    QJsonArray users = data["users"].toArray(data["data"].toArray());
    table->setRowCount(users.size());
    for (int i = 0; i < users.size(); ++i) {
        auto u = users[i].toObject();
        table->setItem(i, 0, new QTableWidgetItem(QString::number(u["id"].toInt())));
        table->setItem(i, 1, new QTableWidgetItem(u["username"].toString()));
        table->setItem(i, 2, new QTableWidgetItem(u["email"].toString()));
        table->setItem(i, 3, new QTableWidgetItem(u["role"].toString()));
        table->setItem(i, 4, new QTableWidgetItem(u["status"].toString(u["is_active"].toBool() ? "Active" : "Inactive")));
    }
}

void MainWindow::populateAdminModules(const QJsonObject& data) {
    auto* table = findChild<QTableWidget*>("adminModulesTable");
    if (!table) return;

    QJsonArray modules = data["modules"].toArray(data["data"].toArray());
    table->setRowCount(modules.size());
    for (int i = 0; i < modules.size(); ++i) {
        auto m = modules[i].toObject();
        table->setItem(i, 0, new QTableWidgetItem(m["name"].toString()));
        table->setItem(i, 1, new QTableWidgetItem(m["version"].toString("--")));

        bool enabled = m["enabled"].toBool(m["is_active"].toBool(true));
        auto* statusItem = new QTableWidgetItem(enabled ? "Enabled" : "Disabled");
        statusItem->setForeground(enabled ? QBrush(QColor("#059669")) : QBrush(QColor("#dc2626")));
        table->setItem(i, 2, statusItem);

        // Toggle button
        auto* toggleBtn = new QPushButton(enabled ? "Disable" : "Enable");
        toggleBtn->setStyleSheet(
            enabled
            ? "QPushButton { background: #dc2626; color: white; border: none; border-radius: 4px; padding: 4px 12px; font-size: 11px; }"
            : "QPushButton { background: #059669; color: white; border: none; border-radius: 4px; padding: 4px 12px; font-size: 11px; }"
        );
        QString modName = m["name"].toString();
        bool newState = !enabled;
        connect(toggleBtn, &QPushButton::clicked, this, [this, modName, newState]() {
            apiManager_->toggleModule(modName, newState);
            statusBar()->showMessage(QString("Toggling module: %1...").arg(modName), 3000);
        });
        table->setCellWidget(i, 3, toggleBtn);
    }
}

void MainWindow::populateAdminMonitor(const QJsonObject& data) {
    auto* label = findChild<QLabel*>("adminMonitorInfo");
    if (!label) return;

    QString cpu = data["cpu_usage"].toString(data["cpu"].toString("--"));
    QString mem = data["memory_usage"].toString(data["memory"].toString("--"));
    QString disk = data["disk_usage"].toString(data["disk"].toString("--"));
    QString connections = data["active_connections"].toString(data["connections"].toString("--"));

    label->setText(
        QString("CPU: %1% | Memory: %2% | Disk: %3% | Connections: %4")
            .arg(cpu, mem, disk, connections)
    );
}

void MainWindow::populatePerformanceMetrics(const QJsonObject& data) {
    // Show as dialog for better visibility
    QDialog dlg(this);
    dlg.setWindowTitle("Performance Metrics");
    dlg.setMinimumSize(600, 400);
    auto* layout = new QVBoxLayout(&dlg);

    auto* info = new QTextEdit();
    info->setReadOnly(true);
    info->setStyleSheet(
        "QTextEdit { background: palette(base); color: palette(text); border: 1px solid palette(mid); "
        "border-radius: 8px; padding: 12px; font-family: monospace; font-size: 12px; }"
    );

    QStringList lines;
    lines << "=== Performance Metrics ===";
    for (auto it = data.begin(); it != data.end(); ++it) {
        if (it.value().isObject()) {
            lines << QString("\n%1:").arg(it.key());
            auto sub = it.value().toObject();
            for (auto sit = sub.begin(); sit != sub.end(); ++sit) {
                lines << QString("  %1: %2").arg(sit.key(), sit.value().toVariant().toString());
            }
        } else if (it.value().isArray()) {
            lines << QString("\n%1: [%2 items]").arg(it.key()).arg(it.value().toArray().size());
        } else {
            lines << QString("%1: %2").arg(it.key(), it.value().toVariant().toString());
        }
    }
    info->setPlainText(lines.join("\n"));
    layout->addWidget(info);

    auto* closeBtn = new QPushButton("Close");
    closeBtn->setStyleSheet(
        "QPushButton { background: palette(button); color: palette(button-text); border: 1px solid palette(mid); "
        "border-radius: 6px; padding: 8px 24px; font-weight: bold; }"
    );
    connect(closeBtn, &QPushButton::clicked, &dlg, &QDialog::close);
    layout->addWidget(closeBtn, 0, Qt::AlignRight);

    dlg.exec();
}

void MainWindow::showLoginHistory(const QJsonObject& data) {
    QDialog dlg(this);
    dlg.setWindowTitle("Login History");
    dlg.setMinimumSize(600, 400);
    auto* layout = new QVBoxLayout(&dlg);

    QJsonArray entries = data["logins"].toArray(data["data"].toArray(data["entries"].toArray()));
    auto* table = new QTableWidget(entries.size(), 4);
    table->setHorizontalHeaderLabels({"Time", "User", "IP", "Status"});
    table->horizontalHeader()->setStretchLastSection(true);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setAlternatingRowColors(true);
    table->setStyleSheet(
        "QTableWidget { background: palette(base); border: 1px solid palette(mid); border-radius: 8px; }"
        "QHeaderView::section { background: palette(window); color: palette(text); "
        "font-weight: bold; padding: 6px; border: none; }"
        "QTableWidget::item { padding: 4px; color: palette(text); }"
    );

    for (int i = 0; i < entries.size(); ++i) {
        auto e = entries[i].toObject();
        table->setItem(i, 0, new QTableWidgetItem(e["timestamp"].toString(e["time"].toString())));
        table->setItem(i, 1, new QTableWidgetItem(e["username"].toString(e["user"].toString())));
        table->setItem(i, 2, new QTableWidgetItem(e["ip_address"].toString(e["ip"].toString())));
        table->setItem(i, 3, new QTableWidgetItem(e["status"].toString(e["action"].toString())));
    }
    layout->addWidget(table);

    auto* closeBtn = new QPushButton("Close");
    closeBtn->setStyleSheet(
        "QPushButton { background: palette(button); color: palette(button-text); border: 1px solid palette(mid); "
        "border-radius: 6px; padding: 8px 24px; font-weight: bold; }"
    );
    connect(closeBtn, &QPushButton::clicked, &dlg, &QDialog::close);
    layout->addWidget(closeBtn, 0, Qt::AlignRight);

    dlg.exec();
}

void MainWindow::populateCrawlerSchedules(const QJsonObject& data) {
    auto* table = findChild<QTableWidget*>("crawlerSchedulesTable");
    if (!table) return;

    QJsonArray schedules = data["schedules"].toArray(data["data"].toArray());
    table->setRowCount(schedules.size());
    for (int i = 0; i < schedules.size(); ++i) {
        auto s = schedules[i].toObject();
        table->setItem(i, 0, new QTableWidgetItem(QString::number(s["id"].toInt())));
        table->setItem(i, 1, new QTableWidgetItem(s["cron"].toString(s["schedule"].toString())));
        table->setItem(i, 2, new QTableWidgetItem(
            s["template_name"].toString(s["template_id"].toString())));

        auto* actions = new QWidget();
        auto* al = new QHBoxLayout(actions);
        al->setContentsMargins(4, 2, 4, 2);
        al->setSpacing(4);
        int sid = s["id"].toInt();

        auto* triggerBtn = new QPushButton("Run");
        triggerBtn->setStyleSheet(
            "QPushButton { background: #059669; color: white; border: none; border-radius: 3px; padding: 2px 8px; font-size: 10px; }"
        );
        connect(triggerBtn, &QPushButton::clicked, this, [this, sid]() {
            apiManager_->triggerCrawlerSchedule(sid);
            statusBar()->showMessage(QString("Triggered schedule %1").arg(sid), 3000);
        });
        al->addWidget(triggerBtn);
        al->addStretch();
        table->setCellWidget(i, 3, actions);
    }
}

void MainWindow::populateCrawlerTemplates(const QJsonObject& data) {
    auto* table = findChild<QTableWidget*>("crawlerTemplatesTable");
    if (!table) return;

    QJsonArray templates = data["templates"].toArray(data["data"].toArray());
    table->setRowCount(templates.size());
    for (int i = 0; i < templates.size(); ++i) {
        auto t = templates[i].toObject();
        table->setItem(i, 0, new QTableWidgetItem(t["name"].toString()));
        table->setItem(i, 1, new QTableWidgetItem(t["source"].toString()));
        table->setItem(i, 2, new QTableWidgetItem(t["url_pattern"].toString().left(40)));

        auto* actions = new QWidget();
        auto* al = new QHBoxLayout(actions);
        al->setContentsMargins(4, 2, 4, 2);
        al->setSpacing(4);
        int tid = t["id"].toInt();

        auto* testBtn = new QPushButton("Test");
        testBtn->setStyleSheet(
            "QPushButton { background: #0891b2; color: white; border: none; border-radius: 3px; padding: 2px 8px; font-size: 10px; }"
        );
        connect(testBtn, &QPushButton::clicked, this, [this, tid]() {
            apiManager_->testCrawlerTemplate(tid);
            statusBar()->showMessage(QString("Testing template %1...").arg(tid), 3000);
        });
        al->addWidget(testBtn);

        auto* delBtn = new QPushButton("Del");
        delBtn->setStyleSheet(
            "QPushButton { background: #dc2626; color: white; border: none; border-radius: 3px; padding: 2px 8px; font-size: 10px; }"
        );
        connect(delBtn, &QPushButton::clicked, this, [this, tid]() {
            apiManager_->deleteCrawlerTemplate(tid);
            QTimer::singleShot(500, this, [this]() { apiManager_->getCrawlerTemplates(); });
        });
        al->addWidget(delBtn);
        al->addStretch();
        table->setCellWidget(i, 3, actions);
    }
}

void MainWindow::populateSavedSearches(const QJsonObject& data) {
    auto* container = findChild<QWidget*>("savedSearchesList");
    if (!container) return;
    auto* layout = container->layout();
    if (!layout) return;

    // Clear old items (keep the stretch at end)
    QLayoutItem* item;
    while (layout->count() > 0) {
        item = layout->takeAt(0);
        delete item->widget();
        delete item;
    }

    QJsonArray searches = data["searches"].toArray(data["data"].toArray(data["saved_searches"].toArray()));
    for (const auto& val : searches) {
        auto s = val.toObject();
        QString query = s["query"].toString(s["keyword"].toString());
        QString name = s["name"].toString(query.left(20));

        auto* chip = new QPushButton(name);
        chip->setStyleSheet(
            "QPushButton { background: #e0e7ff; color: #4338ca; border: none; border-radius: 12px; "
            "padding: 4px 12px; font-size: 11px; font-weight: bold; }"
            "QPushButton:hover { background: #c7d2fe; }"
        );
        chip->setCursor(Qt::PointingHandCursor);
        connect(chip, &QPushButton::clicked, this, [this, query]() {
            onSearch(query);
            searchWidget_->setText(query);
        });
        layout->addWidget(chip);
    }
    layout->addStretch();
}

void MainWindow::populateTrendingSearches(const QJsonObject& data) {
    auto* container = findChild<QWidget*>("trendingSearchesList");
    if (!container) return;
    auto* layout = container->layout();
    if (!layout) return;

    QLayoutItem* item;
    while (layout->count() > 0) {
        item = layout->takeAt(0);
        delete item->widget();
        delete item;
    }

    QJsonArray trending = data["trending"].toArray(data["data"].toArray(data["trending_searches"].toArray()));
    for (const auto& val : trending) {
        QString query;
        if (val.isObject()) {
            query = val.toObject()["query"].toString(val.toObject()["keyword"].toString());
        } else {
            query = val.toString();
        }
        if (query.isEmpty()) continue;

        auto* chip = new QPushButton(query.left(25));
        chip->setStyleSheet(
            "QPushButton { background: #fef3c7; color: #92400e; border: none; border-radius: 12px; "
            "padding: 4px 12px; font-size: 11px; font-weight: bold; }"
            "QPushButton:hover { background: #fde68a; }"
        );
        chip->setCursor(Qt::PointingHandCursor);
        connect(chip, &QPushButton::clicked, this, [this, query]() {
            onSearch(query);
            searchWidget_->setText(query);
        });
        layout->addWidget(chip);
    }
    layout->addStretch();
}

void MainWindow::showStatsDialog(const QString& title, const QJsonObject& data) {
    QDialog dlg(this);
    dlg.setWindowTitle(title);
    dlg.setMinimumSize(700, 500);
    auto* layout = new QVBoxLayout(&dlg);

    // Summary cards at top
    auto* cardsRow = new QHBoxLayout();
    cardsRow->setSpacing(12);
    int cardCount = 0;
    for (auto it = data.begin(); it != data.end() && cardCount < 4; ++it) {
        if (it.value().isObject() || it.value().isArray()) continue;

        auto* card = new QWidget();
        QString bgColors[] = {"#4f46e5", "#0891b2", "#059669", "#d97706"};
        card->setStyleSheet(
            QString("QWidget { background: %1; border-radius: 8px; padding: 12px; }")
                .arg(bgColors[cardCount % 4])
        );
        auto* cl = new QVBoxLayout(card);
        cl->setContentsMargins(8, 6, 8, 6);
        cl->setSpacing(2);

        auto* valLabel = new QLabel(it.value().toVariant().toString());
        valLabel->setStyleSheet("font-size: 20px; font-weight: bold; color: white;");
        cl->addWidget(valLabel);

        auto* nameLabel = new QLabel(it.key().replace("_", " "));
        nameLabel->setStyleSheet("font-size: 10px; color: rgba(255,255,255,0.8);");
        cl->addWidget(nameLabel);

        cardsRow->addWidget(card);
        cardCount++;
    }
    layout->addLayout(cardsRow);

    // Detailed table
    auto* table = new QTableWidget();
    table->setColumnCount(2);
    table->setHorizontalHeaderLabels({"Metric", "Value"});
    table->horizontalHeader()->setStretchLastSection(true);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setAlternatingRowColors(true);
    table->setStyleSheet(
        "QTableWidget { background: palette(base); border: 1px solid palette(mid); border-radius: 8px; }"
        "QHeaderView::section { background: palette(window); color: palette(text); "
        "font-weight: bold; padding: 6px; border: none; }"
        "QTableWidget::item { padding: 4px; color: palette(text); }"
        "QTableWidget::item:alternate { background: palette(alternate-base); }"
    );

    // Flatten nested JSON for table display
    QList<QPair<QString, QString>> flatRows;
    std::function<void(const QJsonObject&, const QString&)> flatten;
    flatten = [&flatten, &flatRows](const QJsonObject& obj, const QString& prefix) {
        for (auto it = obj.begin(); it != obj.end(); ++it) {
            QString key = prefix.isEmpty() ? it.key() : prefix + " > " + it.key();
            if (it.value().isObject()) {
                flatten(it.value().toObject(), key);
            } else if (it.value().isArray()) {
                flatRows.append({key, QString("[%1 items]").arg(it.value().toArray().size())});
            } else {
                flatRows.append({key, it.value().toVariant().toString()});
            }
        }
    };
    flatten(data, "");

    table->setRowCount(flatRows.size());
    for (int i = 0; i < flatRows.size(); ++i) {
        table->setItem(i, 0, new QTableWidgetItem(flatRows[i].first));
        table->setItem(i, 1, new QTableWidgetItem(flatRows[i].second));
    }
    layout->addWidget(table, 1);

    auto* closeBtn = new QPushButton("Close");
    closeBtn->setStyleSheet(
        "QPushButton { background: palette(button); color: palette(button-text); border: 1px solid palette(mid); "
        "border-radius: 6px; padding: 8px 24px; font-weight: bold; }"
    );
    connect(closeBtn, &QPushButton::clicked, &dlg, &QDialog::close);
    layout->addWidget(closeBtn, 0, Qt::AlignRight);

    dlg.exec();
}

void MainWindow::showAiStatusDialog(const QJsonObject& data) {
    QDialog dlg(this);
    dlg.setWindowTitle("AI Service Status");
    dlg.setMinimumSize(500, 400);
    auto* layout = new QVBoxLayout(&dlg);

    // Status indicator
    bool available = data["available"].toBool(data["status"].toString() != "offline");
    QString statusColor = available ? "#059669" : "#dc2626";
    QString statusText = available ? "Online" : "Offline";

    auto* statusRow = new QHBoxLayout();
    auto* dot = new QLabel();
    dot->setFixedSize(16, 16);
    dot->setStyleSheet(QString("background: %1; border-radius: 8px;").arg(statusColor));
    statusRow->addWidget(dot);
    auto* statusLabel = new QLabel("AI Service: " + statusText);
    statusLabel->setStyleSheet(QString("font-size: 16px; font-weight: bold; color: %1;").arg(statusColor));
    statusRow->addWidget(statusLabel);
    statusRow->addStretch();
    layout->addLayout(statusRow);

    // Details table
    auto* table = new QTableWidget();
    table->setColumnCount(2);
    table->setHorizontalHeaderLabels({"Property", "Value"});
    table->horizontalHeader()->setStretchLastSection(true);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setAlternatingRowColors(true);
    table->setStyleSheet(
        "QTableWidget { background: palette(base); border: 1px solid palette(mid); border-radius: 8px; }"
        "QHeaderView::section { background: palette(window); color: palette(text); "
        "font-weight: bold; padding: 6px; border: none; }"
        "QTableWidget::item { padding: 4px; color: palette(text); }"
    );

    QList<QPair<QString, QString>> rows;
    for (auto it = data.begin(); it != data.end(); ++it) {
        if (it.value().isObject() || it.value().isArray()) continue;
        rows.append({it.key().replace("_", " "), it.value().toVariant().toString()});
    }
    table->setRowCount(rows.size());
    for (int i = 0; i < rows.size(); ++i) {
        table->setItem(i, 0, new QTableWidgetItem(rows[i].first));
        table->setItem(i, 1, new QTableWidgetItem(rows[i].second));
    }
    layout->addWidget(table, 1);

    auto* closeBtn = new QPushButton("Close");
    closeBtn->setStyleSheet(
        "QPushButton { background: palette(button); color: palette(button-text); border: 1px solid palette(mid); "
        "border-radius: 6px; padding: 8px 24px; font-weight: bold; }"
    );
    connect(closeBtn, &QPushButton::clicked, &dlg, &QDialog::close);
    layout->addWidget(closeBtn, 0, Qt::AlignRight);

    dlg.exec();
}

void MainWindow::showCrawlerWorkersDialog(const QJsonObject& data) {
    QDialog dlg(this);
    dlg.setWindowTitle("Crawler Workers");
    dlg.setMinimumSize(600, 400);
    auto* layout = new QVBoxLayout(&dlg);

    QJsonArray workers = data["workers"].toArray(data["data"].toArray());
    auto* table = new QTableWidget(workers.size(), 5);
    table->setHorizontalHeaderLabels({"ID", "Status", "Task", "Uptime", "Papers Crawled"});
    table->horizontalHeader()->setStretchLastSection(true);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setAlternatingRowColors(true);
    table->setStyleSheet(
        "QTableWidget { background: palette(base); border: 1px solid palette(mid); border-radius: 8px; }"
        "QHeaderView::section { background: palette(window); color: palette(text); "
        "font-weight: bold; padding: 6px; border: none; }"
        "QTableWidget::item { padding: 4px; color: palette(text); }"
    );

    for (int i = 0; i < workers.size(); ++i) {
        auto w = workers[i].toObject();
        table->setItem(i, 0, new QTableWidgetItem(QString::number(w["id"].toInt())));
        QString status = w["status"].toString("unknown");
        auto* statusItem = new QTableWidgetItem(status);
        statusItem->setForeground(status == "active" ? QBrush(QColor("#059669")) : QBrush(QColor("#6b7280")));
        table->setItem(i, 1, statusItem);
        table->setItem(i, 2, new QTableWidgetItem(QString::number(w["task_id"].toInt(w["current_task"].toInt()))));
        table->setItem(i, 3, new QTableWidgetItem(w["uptime"].toString(w["running_since"].toString("--")));
        table->setItem(i, 4, new QTableWidgetItem(QString::number(w["papers_crawled"].toInt(w["items_processed"].toInt(0)))));
    }
    layout->addWidget(table, 1);

    // Summary
    int activeCount = 0;
    for (const auto& w : workers) {
        if (w.toObject()["status"].toString() == "active") activeCount++;
    }
    auto* summary = new QLabel(QString("Total: %1 workers | Active: %2").arg(workers.size()).arg(activeCount));
    summary->setStyleSheet("color: palette(mid); font-size: 12px; padding: 8px;");
    layout->addWidget(summary);

    auto* closeBtn = new QPushButton("Close");
    closeBtn->setStyleSheet(
        "QPushButton { background: palette(button); color: palette(button-text); border: 1px solid palette(mid); "
        "border-radius: 6px; padding: 8px 24px; font-weight: bold; }"
    );
    connect(closeBtn, &QPushButton::clicked, &dlg, &QDialog::close);
    layout->addWidget(closeBtn, 0, Qt::AlignRight);
    dlg.exec();
}

void MainWindow::showCrawlerStatisticsDialog(const QJsonObject& data) {
    showStatsDialog("Crawler Statistics", data);
}

void MainWindow::showSearchHistoryDialog(const QJsonObject& data) {
    QDialog dlg(this);
    dlg.setWindowTitle("Search History");
    dlg.setMinimumSize(600, 400);
    auto* layout = new QVBoxLayout(&dlg);

    QJsonArray history = data["history"].toArray(data["data"].toArray(data["searches"].toArray()));
    auto* table = new QTableWidget(history.size(), 3);
    table->setHorizontalHeaderLabels({"Query", "Results", "Time"});
    table->horizontalHeader()->setStretchLastSection(true);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setAlternatingRowColors(true);
    table->setStyleSheet(
        "QTableWidget { background: palette(base); border: 1px solid palette(mid); border-radius: 8px; }"
        "QHeaderView::section { background: palette(window); color: palette(text); "
        "font-weight: bold; padding: 6px; border: none; }"
        "QTableWidget::item { padding: 4px; color: palette(text); }"
    );

    for (int i = 0; i < history.size(); ++i) {
        auto h = history[i].toObject();
        QString query = h["query"].toString(h["keyword"].toString());
        auto* queryItem = new QTableWidgetItem(query);
        queryItem->setForeground(QBrush(QColor("#4f46e5")));
        queryItem->setCursor(Qt::PointingHandCursor);
        table->setItem(i, 0, queryItem);
        table->setItem(i, 1, new QTableWidgetItem(QString::number(h["result_count"].toInt(h["total"].toInt(0)))));
        table->setItem(i, 2, new QTableWidgetItem(h["created_at"].toString(h["timestamp"].toString(h["time"].toString()))));
    }

    connect(table, &QTableWidget::cellDoubleClicked, this, [this, table, history](int row, int) {
        if (row >= 0 && row < history.size()) {
            QString query = history[row].toObject()["query"].toString(history[row].toObject()["keyword"].toString());
            if (!query.isEmpty()) {
                onSearch(query);
                searchWidget_->setText(query);
            }
        }
    });

    layout->addWidget(table, 1);

    auto* hint = new QLabel("Double-click a row to search again");
    hint->setStyleSheet("color: palette(mid); font-size: 11px; padding: 4px;");
    layout->addWidget(hint);

    auto* closeBtn = new QPushButton("Close");
    closeBtn->setStyleSheet(
        "QPushButton { background: palette(button); color: palette(button-text); border: 1px solid palette(mid); "
        "border-radius: 6px; padding: 8px 24px; font-weight: bold; }"
    );
    connect(closeBtn, &QPushButton::clicked, &dlg, &QDialog::close);
    layout->addWidget(closeBtn, 0, Qt::AlignRight);
    dlg.exec();
}

void MainWindow::showAllPapersDialog(const QJsonObject& data) {
    QDialog dlg(this);
    dlg.setWindowTitle("All Papers");
    dlg.setMinimumSize(800, 550);
    auto* layout = new QVBoxLayout(&dlg);

    QJsonArray papers = data["papers"].toArray(data["data"].toArray());
    int total = data["total"].toInt(papers.size());

    auto* infoLabel = new QLabel(QString("Showing %1 of %2 papers").arg(papers.size()).arg(total));
    infoLabel->setStyleSheet("color: palette(mid); font-size: 12px; font-weight: bold;");
    layout->addWidget(infoLabel);

    auto* table = new QTableWidget(papers.size(), 5);
    table->setHorizontalHeaderLabels({"ID", "Title", "Journal", "Year", "Level"});
    table->horizontalHeader()->setStretchLastSection(true);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setAlternatingRowColors(true);
    table->setStyleSheet(
        "QTableWidget { background: palette(base); border: 1px solid palette(mid); border-radius: 8px; }"
        "QHeaderView::section { background: palette(window); color: palette(text); "
        "font-weight: bold; padding: 6px; border: none; }"
        "QTableWidget::item { padding: 4px; color: palette(text); }"
    );

    for (int i = 0; i < papers.size(); ++i) {
        auto p = papers[i].toObject();
        table->setItem(i, 0, new QTableWidgetItem(QString::number(p["id"].toInt())));
        table->setItem(i, 1, new QTableWidgetItem(p["title"].toString().left(80)));
        table->setItem(i, 2, new QTableWidgetItem(p["journal"].toString(p["journal_name"].toString())));
        table->setItem(i, 3, new QTableWidgetItem(p["year"].toString()));
        table->setItem(i, 4, new QTableWidgetItem(p["level"].toString()));
    }

    connect(table, &QTableWidget::cellDoubleClicked, this, [this, papers](int row, int) {
        if (row >= 0 && row < papers.size()) {
            int pid = papers[row].toObject()["id"].toInt();
            onPaperSelected(pid);
        }
    });

    layout->addWidget(table, 1);

    auto* hint = new QLabel("Double-click to view paper details");
    hint->setStyleSheet("color: palette(mid); font-size: 11px; padding: 4px;");
    layout->addWidget(hint);

    auto* closeBtn = new QPushButton("Close");
    closeBtn->setStyleSheet(
        "QPushButton { background: palette(button); color: palette(button-text); border: 1px solid palette(mid); "
        "border-radius: 6px; padding: 8px 24px; font-weight: bold; }"
    );
    connect(closeBtn, &QPushButton::clicked, &dlg, &QDialog::close);
    layout->addWidget(closeBtn, 0, Qt::AlignRight);
    dlg.exec();
}

void MainWindow::showRecommendationExplanationDialog(const QJsonObject& data) {
    QDialog dlg(this);
    dlg.setWindowTitle("Why this recommendation?");
    dlg.setMinimumSize(500, 350);
    auto* layout = new QVBoxLayout(&dlg);

    QString title = data["title"].toString(data["paper_title"].toString("Paper"));
    auto* titleLabel = new QLabel("Why we recommend: " + title.left(60));
    titleLabel->setWordWrap(true);
    titleLabel->setStyleSheet("font-size: 16px; font-weight: bold; color: palette(text);");
    layout->addWidget(titleLabel);

    auto* divider = new QFrame();
    divider->setFrameShape(QFrame::HLine);
    divider->setStyleSheet("color: palette(mid);");
    layout->addWidget(divider);

    // Explanation
    QString explanation = data["explanation"].toString(data["reason"].toString(data["text"].toString())));
    if (!explanation.isEmpty()) {
        auto* expLabel = new QLabel(explanation);
        expLabel->setWordWrap(true);
        expLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
        expLabel->setStyleSheet("color: palette(text); font-size: 14px; line-height: 1.5; padding: 8px;");
        layout->addWidget(expLabel, 1);
    }

    // Score
    QString score = data["score"].toString(data["relevance_score"].toString());
    if (!score.isEmpty()) {
        auto* scoreLabel = new QLabel("Relevance Score: " + score.left(6));
        scoreLabel->setStyleSheet("font-size: 13px; font-weight: bold; color: #4f46e5; padding: 8px;");
        layout->addWidget(scoreLabel);
    }

    // Similar factors
    QJsonArray factors = data["factors"].toArray(data["reasons"].toArray());
    if (!factors.isEmpty()) {
        auto* factorsLabel = new QLabel("Key Factors:");
        factorsLabel->setStyleSheet("font-weight: bold; color: palette(text); font-size: 12px; margin-top: 8px;");
        layout->addWidget(factorsLabel);
        for (const auto& f : factors) {
            QString factorText = f.isObject() ? f.toObject()["name"].toString(f.toObject()["text"].toString()) : f.toString();
            if (!factorText.isEmpty()) {
                auto* fLabel = new QLabel("  - " + factorText);
                fLabel->setStyleSheet("color: palette(mid); font-size: 12px;");
                layout->addWidget(fLabel);
            }
        }
    }

    layout->addStretch();

    auto* closeBtn = new QPushButton("Close");
    closeBtn->setStyleSheet(
        "QPushButton { background: palette(button); color: palette(button-text); border: 1px solid palette(mid); "
        "border-radius: 6px; padding: 8px 24px; font-weight: bold; }"
    );
    connect(closeBtn, &QPushButton::clicked, &dlg, &QDialog::close);
    layout->addWidget(closeBtn, 0, Qt::AlignRight);
    dlg.exec();
}

void MainWindow::showLatexTemplatesDialog(const QJsonObject& data) {
    QDialog dlg(this);
    dlg.setWindowTitle("LaTeX Templates");
    dlg.setMinimumSize(600, 450);
    auto* layout = new QVBoxLayout(&dlg);

    QJsonArray templates = data["templates"].toArray(data["data"].toArray());
    if (templates.isEmpty()) {
        auto* emptyLabel = new QLabel("No templates available.\nCreate documents from scratch or add templates on the server.");
        emptyLabel->setAlignment(Qt::AlignCenter);
        emptyLabel->setStyleSheet("color: palette(mid); font-size: 14px; padding: 40px;");
        layout->addWidget(emptyLabel);
        auto* closeBtn = new QPushButton("Close");
        closeBtn->setStyleSheet("QPushButton { background: palette(button); color: palette(button-text); border: 1px solid palette(mid); border-radius: 6px; padding: 8px 24px; font-weight: bold; }");
        connect(closeBtn, &QPushButton::clicked, &dlg, &QDialog::close);
        layout->addWidget(closeBtn, 0, Qt::AlignRight);
        dlg.exec();
        return;
    }

    auto* table = new QTableWidget(templates.size(), 3);
    table->setHorizontalHeaderLabels({"Name", "Category", "Action"});
    table->horizontalHeader()->setStretchLastSection(true);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setAlternatingRowColors(true);
    table->setStyleSheet(
        "QTableWidget { background: palette(base); border: 1px solid palette(mid); border-radius: 8px; }"
        "QHeaderView::section { background: palette(window); color: palette(text); font-weight: bold; padding: 6px; border: none; }"
        "QTableWidget::item { padding: 4px; color: palette(text); }"
    );

    for (int i = 0; i < templates.size(); ++i) {
        auto t = templates[i].toObject();
        table->setItem(i, 0, new QTableWidgetItem(t["name"].toString()));
        table->setItem(i, 1, new QTableWidgetItem(t["category"].toString()));

        auto* applyBtn = new QPushButton("Apply");
        applyBtn->setStyleSheet(
            "QPushButton { background: #4f46e5; color: white; border: none; border-radius: 4px; padding: 4px 12px; font-size: 11px; }"
            "QPushButton:hover { background: #4338ca; }"
        );
        QString content = t["content"].toString(t["template"].toString());
        connect(applyBtn, &QPushButton::clicked, this, [this, &dlg, content]() {
            if (latexEditor_) {
                latexEditor_->loadContent(content);
            }
            dlg.accept();
            statusBar()->showMessage("Template applied", 3000);
        });
        table->setCellWidget(i, 2, applyBtn);
    }
    layout->addWidget(table, 1);

    auto* closeBtn = new QPushButton("Close");
    closeBtn->setStyleSheet(
        "QPushButton { background: palette(button); color: palette(button-text); border: 1px solid palette(mid); "
        "border-radius: 6px; padding: 8px 24px; font-weight: bold; }"
    );
    connect(closeBtn, &QPushButton::clicked, &dlg, &QDialog::close);
    layout->addWidget(closeBtn, 0, Qt::AlignRight);
    dlg.exec();
}

void MainWindow::createMenus() {
    // File menu
    QMenu* fileMenu = menuBar()->addMenu("&File");

    // Export sub-menu
    QMenu* exportMenu = fileMenu->addMenu("&Export");

    QAction* exportCSVAction = exportMenu->addAction("Export to &CSV");
    exportCSVAction->setShortcut(QKeySequence("Ctrl+E"));
    connect(exportCSVAction, &QAction::triggered, this, [this]() {
        onExport(ExportFormat::CSV);
    });

    QAction* exportBibTeXAction = exportMenu->addAction("Export to &BibTeX");
    exportBibTeXAction->setShortcut(QKeySequence("Ctrl+Shift+E"));
    connect(exportBibTeXAction, &QAction::triggered, this, [this]() {
        onExport(ExportFormat::BibTeX);
    });

    QAction* exportJSONAction = exportMenu->addAction("Export to &JSON");
    exportJSONAction->setShortcut(QKeySequence("Ctrl+Alt+E"));
    connect(exportJSONAction, &QAction::triggered, this, [this]() {
        onExport(ExportFormat::JSON);
    });

    fileMenu->addSeparator();

    QAction* exitAction = fileMenu->addAction("E&xit");
    exitAction->setShortcut(QKeySequence("Ctrl+Q"));
    connect(exitAction, &QAction::triggered, this, &QMainWindow::close);

    // Edit menu
    QMenu* editMenu = menuBar()->addMenu("&Edit");

    QAction* preferencesAction = editMenu->addAction("&Preferences");
    preferencesAction->setShortcut(QKeySequence("Ctrl+P"));
    connect(preferencesAction, &QAction::triggered, this, &MainWindow::onPreferences);

    editMenu->addSeparator();

    auto* advSearchAction = editMenu->addAction("&Advanced Search");
    advSearchAction->setShortcut(QKeySequence("Ctrl+Shift+F"));
    connect(advSearchAction, &QAction::triggered, this, [this]() {
        auto* dlg = new AdvancedSearchDialog(this);
        connect(dlg, &AdvancedSearchDialog::searchRequested, this, [this](const AdvancedSearchDialog::SearchCriteria& c) {
            QString query = c.query;
            if (!c.title.isEmpty()) query += QString(" title:\"%1\"").arg(c.title);
            if (!c.author.isEmpty()) query += QString(" author:\"%1\"").arg(c.author);
            if (!c.venue.isEmpty()) query += QString(" venue:\"%1\"").arg(c.venue);
            if (!c.yearFrom.isEmpty()) query += QString(" year:>=%1").arg(c.yearFrom);
            if (!c.yearTo.isEmpty()) query += QString(" year:<=%1").arg(c.yearTo);
            if (!c.level.isEmpty()) query += QString(" level:%1").arg(c.level);
            onSearch(query.trimmed());
            tabWidget_->setCurrentIndex(0);
            ToastWidget::showInfo("Searching...");
        });
        dlg->exec();
        dlg->deleteLater();
    });

    // View menu
    QMenu* viewMenu = menuBar()->addMenu("&View");

    QAction* themeAction = viewMenu->addAction("&Toggle Theme");
    themeAction->setShortcut(QKeySequence("Ctrl+T"));
    connect(themeAction, &QAction::triggered, this, &MainWindow::onToggleTheme);

    QAction* themeCustomAction = viewMenu->addAction("Theme &Customizer");
    connect(themeCustomAction, &QAction::triggered, this, [this]() {
        if (!themeCustomizer_) {
            themeCustomizer_ = new ThemeCustomizer(this);
            connect(themeCustomizer_, &ThemeCustomizer::themeChanged, this,
                    [this](const QMap<QString, QColor>& colors) {
                Q_UNUSED(colors);
                ToastWidget::showSuccess("Custom theme applied");
            });
            connect(themeCustomizer_, &ThemeCustomizer::resetToDefault, this, [this]() {
                ToastWidget::showInfo("Theme reset to default");
            });
        }
        themeCustomizer_->exec();
    });

    // Tools menu
    QMenu* toolsMenu = menuBar()->addMenu("&Tools");

    QAction* statsAction = toolsMenu->addAction("&Statistics");
    statsAction->setShortcut(QKeySequence("Ctrl+S"));
    connect(statsAction, &QAction::triggered, this, &MainWindow::onShowStatistics);

    toolsMenu->addSeparator();

    QAction* crawlerAction = toolsMenu->addAction("&Crawler Dashboard");
    connect(crawlerAction, &QAction::triggered, this, [this]() {
        tabWidget_->setCurrentIndex(2);
        apiManager_->getCrawlerDashboard();
    });

    QAction* aiAction = toolsMenu->addAction("&AI Assistant");
    connect(aiAction, &QAction::triggered, this, [this]() {
        tabWidget_->setCurrentIndex(3);
    });

    toolsMenu->addSeparator();

    auto* readingListAction = toolsMenu->addAction("&Reading Lists");
    connect(readingListAction, &QAction::triggered, this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Reading Lists");
        dlg->resize(700, 450);
        auto* layout = new QVBoxLayout(dlg);
        auto* mgr = new ReadingListManager(apiManager_);
        layout->addWidget(mgr);
        dlg->exec();
        dlg->deleteLater();
    });

    auto* cacheAction = toolsMenu->addAction("&Offline Cache");
    connect(cacheAction, &QAction::triggered, this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Offline Cache Manager");
        dlg->resize(800, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* mgr = new OfflineCacheManager(localDb_);
        mgr->refreshPapers();
        layout->addWidget(mgr);
        dlg->exec();
        dlg->deleteLater();
    });

    auto* historyAction = toolsMenu->addAction("Recent &History");
    historyAction->setShortcut(QKeySequence("Ctrl+H"));
    connect(historyAction, &QAction::triggered, this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Recently Viewed Papers");
        dlg->resize(400, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* hist = new RecentHistoryWidget();
        connect(hist, &RecentHistoryWidget::openPaperRequested, this, [this](int paperId) {
            apiManager_->getPaperDetails(paperId);
        });
        layout->addWidget(hist);
        dlg->exec();
        dlg->deleteLater();
    });

    toolsMenu->addSeparator();

    auto* shortcutAction = toolsMenu->addAction("Keyboard &Shortcuts");
    connect(shortcutAction, &QAction::triggered, this, [this]() {
        auto* dlg = new ShortcutConfigDialog(this);
        dlg->exec();
        dlg->deleteLater();
    });

    auto* timelineAction = toolsMenu->addAction("Paper &Timeline");
    connect(timelineAction, &QAction::triggered, this, [this]() {
        if (!resultView_) return;
        auto papers = resultView_->getPapers();
        if (papers.isEmpty()) {
            ToastWidget::showWarning("No papers to display. Search first.");
            return;
        }
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Paper Timeline");
        dlg->resize(900, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* timeline = new PaperTimelineWidget();
        timeline->setPapers(papers);
        connect(timeline, &PaperTimelineWidget::paperClicked, this, [this](int paperId) {
            apiManager_->getPaperDetails(paperId);
        });
        layout->addWidget(timeline);
        dlg->exec();
        dlg->deleteLater();
    });

    auto* graphAction = toolsMenu->addAction("Citation &Graph");
    connect(graphAction, &QAction::triggered, this, [this]() {
        if (!resultView_) return;
        auto papers = resultView_->getPapers();
        if (papers.isEmpty()) {
            ToastWidget::showWarning("No papers to display. Search first.");
            return;
        }
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Citation Graph");
        dlg->resize(900, 600);
        auto* layout = new QVBoxLayout(dlg);
        auto* graph = new CitationGraphWidget();
        graph->setPapers(papers);
        connect(graph, &CitationGraphWidget::paperClicked, this, [this, graph](int paperId) {
            apiManager_->getPaperDetails(paperId);
            graph->setFocusPaper(paperId);
        });
        layout->addWidget(graph);
        dlg->exec();
        dlg->deleteLater();
    });

    toolsMenu->addSeparator();

    auto* exportDialogAction = toolsMenu->addAction("Advanced &Export");
    connect(exportDialogAction, &QAction::triggered, this, [this]() {
        if (!resultView_) return;
        auto papers = resultView_->getPapers();
        if (papers.isEmpty()) {
            ToastWidget::showWarning("No papers to export. Search first.");
            return;
        }
        auto* dlg = new PaperExportDialog(papers, this);
        dlg->exec();
        dlg->deleteLater();
    });

    auto* statsChartAction = toolsMenu->addAction("Paper &Charts");
    connect(statsChartAction, &QAction::triggered, this, [this]() {
        if (!resultView_) return;
        auto papers = resultView_->getPapers();
        if (papers.isEmpty()) {
            ToastWidget::showWarning("No papers. Search first.");
            return;
        }
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Paper Statistics Charts");
        dlg->resize(900, 500);
        auto* layout = new QHBoxLayout(dlg);

        // Year distribution bar chart
        QMap<QString, double> yearData;
        for (const auto& p : papers) {
            QString y = p.year.isEmpty() ? "N/A" : p.year;
            yearData[y] = yearData.value(y, 0) + 1;
        }
        auto* yearChart = new PaperStatsChart();
        yearChart->setChartData(yearData, PaperStatsChart::BarChart);
        yearChart->setTitle("Papers by Year");
        layout->addWidget(yearChart, 1);

        // Source distribution pie chart
        QMap<QString, double> sourceData;
        for (const auto& p : papers) {
            QString s = p.source.isEmpty() ? "Unknown" : p.source;
            sourceData[s] = sourceData.value(s, 0) + 1;
        }
        auto* sourceChart = new PaperStatsChart();
        sourceChart->setChartData(sourceData, PaperStatsChart::PieChart);
        sourceChart->setTitle("Papers by Source");
        layout->addWidget(sourceChart, 1);

        dlg->exec();
        dlg->deleteLater();
    });

    auto* quickNoteAction = toolsMenu->addAction("&Quick Notes");
    quickNoteAction->setShortcut(QKeySequence("Ctrl+Shift+N"));
    connect(quickNoteAction, &QAction::triggered, this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Quick Notes");
        dlg->resize(400, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* notes = new QuickNoteWidget();
        layout->addWidget(notes);
        dlg->exec();
        dlg->deleteLater();
    });

    auto* collectionAction = toolsMenu->addAction("Paper &Collections");
    connect(collectionAction, &QAction::triggered, this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Paper Collections");
        dlg->resize(800, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* collWidget = new PaperCollectionWidget();
        layout->addWidget(collWidget);
        dlg->exec();
        dlg->deleteLater();
    });

    auto* diffAction = toolsMenu->addAction("Side-by-Side &Diff");
    connect(diffAction, &QAction::triggered, this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Side-by-Side Diff");
        dlg->resize(900, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* diff = new SideBySideDiff();
        layout->addWidget(diff);

        QString text1 = QInputDialog::getMultiLineText(this, "Left Content", "Enter left text:");
        QString text2 = QInputDialog::getMultiLineText(this, "Right Content", "Enter right text:");
        diff->setContents("Left", text1, "Right", text2);
        dlg->exec();
        dlg->deleteLater();
    });

    toolsMenu->addSeparator();

    auto* progressAction = toolsMenu->addAction("Reading &Progress");
    connect(progressAction, &QAction::triggered, this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Reading Progress");
        dlg->resize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        progressTracker_ = new ProgressTracker();
        layout->addWidget(progressTracker_);
        dlg->exec();
        dlg->deleteLater();
    });

    auto* backupAction = toolsMenu->addAction("&Backup / Restore");
    connect(backupAction, &QAction::triggered, this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Backup & Restore");
        dlg->resize(600, 450);
        auto* layout = new QVBoxLayout(dlg);
        auto* backup = new BackupRestoreWidget();
        layout->addWidget(backup);
        dlg->exec();
        dlg->deleteLater();
    });

    auto* recommendAction = toolsMenu->addAction("&Recommendations");
    connect(recommendAction, &QAction::triggered, this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Paper Recommendations");
        dlg->resize(500, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* engine = new PaperRecommendationEngine();
        if (recentHistory_) {
            // In real implementation, pass user history data
        }
        layout->addWidget(engine);
        dlg->exec();
        dlg->deleteLater();
    });

    toolsMenu->addSeparator();

    auto* batchImportAction = toolsMenu->addAction("Batch &Import");
    connect(batchImportAction, &QAction::triggered, this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Batch Import");
        dlg->resize(800, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* importWidget = new BatchImportWidget();
        importWidget->setApiManager(apiManager_);
        layout->addWidget(importWidget);
        dlg->exec();
        dlg->deleteLater();
    });

    auto* tagMgrAction = toolsMenu->addAction("Tag &Manager");
    connect(tagMgrAction, &QAction::triggered, this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Tag Manager");
        dlg->resize(700, 500);
        auto* layout = new QVBoxLayout(dlg);
        layout->addWidget(new TagManager());
        dlg->exec();
        dlg->deleteLater();
    });

    auto* workspaceAction = toolsMenu->addAction("&Workspaces");
    connect(workspaceAction, &QAction::triggered, this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Workspace Manager");
        dlg->resize(500, 400);
        auto* layout = new QVBoxLayout(dlg);
        auto* ws = new WorkspaceManager();
        connect(ws, &WorkspaceManager::layoutRestored, this, [this](const WorkspaceLayout& wl) {
            if (!wl.windowGeometry.isEmpty()) restoreGeometry(wl.windowGeometry);
            if (wl.activeTab >= 0) tabWidget_->setCurrentIndex(wl.activeTab);
        });
        layout->addWidget(ws);
        dlg->exec();
        dlg->deleteLater();
    });

    auto* annotationAction = toolsMenu->addAction("&Annotations");
    connect(annotationAction, &QAction::triggered, this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Annotations");
        dlg->resize(800, 500);
        auto* layout = new QVBoxLayout(dlg);
        layout->addWidget(new AnnotationWidget());
        dlg->exec();
        dlg->deleteLater();
    });

    auto* versionAction = toolsMenu->addAction("Version &History");
    connect(versionAction, &QAction::triggered, this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Version History");
        dlg->resize(600, 450);
        auto* layout = new QVBoxLayout(dlg);
        layout->addWidget(new PaperVersionHistory());
        dlg->exec();
        dlg->deleteLater();
    });

    toolsMenu->addSeparator();

    auto* rankingAction = toolsMenu->addAction("Paper &Rankings");
    connect(rankingAction, &QAction::triggered, this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Paper Rankings");
        dlg->resize(700, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* ranking = new PaperRankingWidget();
        if (resultView_) {
            QList<RankingEntry> entries;
            for (const auto& p : resultView_->getPapers()) {
                RankingEntry e;
                e.paperId = p.id;
                e.title = p.title;
                e.authors = p.authors;
                e.year = p.year;
                entries.append(e);
            }
            ranking->setRankings(entries);
        }
        layout->addWidget(ranking);
        dlg->exec();
        dlg->deleteLater();
    });

    auto* aiSummaryAction = toolsMenu->addAction("AI &Summarizer");
    connect(aiSummaryAction, &QAction::triggered, this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("AI Summarizer");
        dlg->resize(900, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* summarizer = new AiSummarizerWidget();
        summarizer->setApiManager(apiManager_);
        layout->addWidget(summarizer);
        dlg->exec();
        dlg->deleteLater();
    });

    auto* journalAction = toolsMenu->addAction("Journal &Browser");
    connect(journalAction, &QAction::triggered, this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Journal Browser");
        dlg->resize(800, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* browser = new JournalBrowserWidget();
        connect(browser, &JournalBrowserWidget::searchPapersInJournal,
                this, [this](const QString& name) {
            onSearch(name);
            tabWidget_->setCurrentIndex(0);
        });
        layout->addWidget(browser);
        dlg->exec();
        dlg->deleteLater();
    });

    auto* feedAction = toolsMenu->addAction("Paper &Feed");
    connect(feedAction, &QAction::triggered, this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Paper Feed");
        dlg->resize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* feed = new PaperFeedWidget();
        connect(feed, &PaperFeedWidget::paperClicked, this, [this](int paperId) {
            apiManager_->getPaperDetails(paperId);
        });
        layout->addWidget(feed);
        dlg->exec();
        dlg->deleteLater();
    });

    auto* hotkeyAction = toolsMenu->addAction("Hotkey &Manager");
    connect(hotkeyAction, &QAction::triggered, this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Hotkey Manager");
        dlg->resize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* mgr = new HotkeyManager();
        mgr->registerAction("search", "Search", "File", QKeySequence("Ctrl+S"));
        mgr->registerAction("export", "Export", "File", QKeySequence("Ctrl+E"));
        mgr->registerAction("theme", "Toggle Theme", "View", QKeySequence("Ctrl+T"));
        mgr->registerAction("settings", "Settings", "Edit", QKeySequence("Ctrl+,"));
        mgr->registerAction("quit", "Quit", "File", QKeySequence("Ctrl+Q"));
        mgr->registerAction("command_palette", "Command Palette", "View", QKeySequence("Ctrl+K"));
        layout->addWidget(mgr);
        dlg->exec();
        dlg->deleteLater();
    });

    toolsMenu->addSeparator();

    auto* citeAction = toolsMenu->addAction("Citation &Exporter");
    connect(citeAction, &QAction::triggered, this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Citation Exporter");
        dlg->resize(800, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* exporter = new CitationExporter();
        if (resultView_) exporter->setPapers(resultView_->getPapers());
        layout->addWidget(exporter);
        dlg->exec();
        dlg->deleteLater();
    });

    auto* queueAction = toolsMenu->addAction("Reading &Queue");
    connect(queueAction, &QAction::triggered, this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Reading Queue");
        dlg->resize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* queue = new ReadingQueueWidget();
        connect(queue, &ReadingQueueWidget::paperClicked, this, [this](int paperId) {
            apiManager_->getPaperDetails(paperId);
        });
        layout->addWidget(queue);
        dlg->exec();
        dlg->deleteLater();
    });

    auto* collabAction = toolsMenu->addAction("&Collaboration");
    connect(collabAction, &QAction::triggered, this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Collaboration");
        dlg->resize(900, 550);
        auto* layout = new QVBoxLayout(dlg);
        layout->addWidget(new CollaborationWidget());
        dlg->exec();
        dlg->deleteLater();
    });

    auto* matrixAction = toolsMenu->addAction("Comparison &Matrix");
    connect(matrixAction, &QAction::triggered, this, [this]() {
        if (!resultView_) return;
        auto papers = resultView_->getPapers();
        if (papers.size() < 2) {
            ToastWidget::showWarning("Need at least 2 papers to compare");
            return;
        }
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Paper Comparison Matrix");
        dlg->resize(900, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* matrix = new PaperComparisonMatrix();
        matrix->setPapers(papers);
        connect(matrix, &PaperComparisonMatrix::paperClicked, this, [this](int paperId) {
            apiManager_->getPaperDetails(paperId);
        });
        layout->addWidget(matrix);
        dlg->exec();
        dlg->deleteLater();
    });

    auto* pdfAction = toolsMenu->addAction("PDF &Viewer");
    connect(pdfAction, &QAction::triggered, this, [this]() {
        auto path = QFileDialog::getOpenFileName(this, "Open PDF", "", "PDF (*.pdf)");
        if (path.isEmpty()) return;
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("PDF Viewer");
        dlg->resize(800, 600);
        auto* layout = new QVBoxLayout(dlg);
        auto* viewer = new PdfViewerWidget();
        viewer->loadFile(path);
        layout->addWidget(viewer);
        dlg->exec();
        dlg->deleteLater();
    });

    toolsMenu->addSeparator();

    auto* smartSearchAction = toolsMenu->addAction("&Smart Search");
    smartSearchAction->setShortcut(QKeySequence("Ctrl+Shift+S"));
    connect(smartSearchAction, &QAction::triggered, this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Smart Search");
        dlg->resize(700, 400);
        auto* layout = new QVBoxLayout(dlg);
        auto* ss = new SmartSearchWidget();
        connect(ss, &SmartSearchWidget::searchRequested, this, [this](const QString& q) {
            onSearch(q);
            tabWidget_->setCurrentIndex(0);
        });
        layout->addWidget(ss);
        dlg->exec();
        dlg->deleteLater();
    });

    auto* clusterAction = toolsMenu->addAction("Paper &Clustering");
    connect(clusterAction, &QAction::triggered, this, [this]() {
        if (!resultView_) return;
        auto papers = resultView_->getPapers();
        if (papers.isEmpty()) {
            ToastWidget::showWarning("No papers. Search first.");
            return;
        }
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Paper Clustering");
        dlg->resize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* clustering = new PaperClusteringWidget();
        QList<QPair<int, QString>> pool;
        for (const auto& p : papers) {
            pool.append({p.id, p.authors + " " + p.journal + " " + p.keywords.join(" ")});
        }
        clustering->setPapers(pool);
        connect(clustering, &PaperClusteringWidget::paperClicked, this, [this](int id) {
            apiManager_->getPaperDetails(id);
        });
        layout->addWidget(clustering);
        dlg->exec();
        dlg->deleteLater();
    });

    auto* scheduledAction = toolsMenu->addAction("Scheduled &Tasks");
    connect(scheduledAction, &QAction::triggered, this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Scheduled Tasks");
        dlg->resize(700, 450);
        auto* layout = new QVBoxLayout(dlg);
        auto* tasks = new ScheduledTaskWidget();
        connect(tasks, &ScheduledTaskWidget::taskExecuted, this, [this](int taskId) {
            ToastWidget::showInfo(QString("Task #%1 executed").arg(taskId));
        });
        layout->addWidget(tasks);
        dlg->exec();
        dlg->deleteLater();
    });

    auto* profileAction = toolsMenu->addAction("User &Profile");
    connect(profileAction, &QAction::triggered, this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("User Profile");
        dlg->resize(500, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* profile = new UserProfileWidget();
        connect(profile, &UserProfileWidget::profileUpdated, this, [](const UserProfile& p) {
            Q_UNUSED(p);
        });
        layout->addWidget(profile);
        dlg->exec();
        dlg->deleteLater();
    });

    auto* browserAction = toolsMenu->addAction("Mini &Browser");
    connect(browserAction, &QAction::triggered, this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Mini Browser");
        dlg->resize(800, 600);
        auto* layout = new QVBoxLayout(dlg);
        auto* browser = new MiniBrowserWidget();
        layout->addWidget(browser);
        dlg->exec();
        dlg->deleteLater();
    });

    auto* doiAction = toolsMenu->addAction("&DOI Lookup");
    connect(doiAction, &QAction::triggered, this, [this]() {
        auto* dlg = new DoiLookupDialog(this);
        connect(dlg, &DoiLookupDialog::paperFound, this, [this](const QJsonObject& data) {
            apiManager_->createPaper(data);
            ToastWidget::showSuccess("Paper imported via DOI");
        });
        dlg->exec();
        dlg->deleteLater();
    });

    auto* dedupAction = toolsMenu->addAction("Find &Duplicates");
    connect(dedupAction, &QAction::triggered, this, [this]() {
        if (!resultView_) return;
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Paper Deduplicator");
        dlg->resize(900, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* dedup = new PaperDeduplicator();
        dedup->setPapers(resultView_->getPapers());
        connect(dedup, &PaperDeduplicator::mergeRequested, this, [this](int keepId, int removeId) {
            apiManager_->deletePaper(removeId);
            Q_UNUSED(keepId);
        });
        connect(dedup, &PaperDeduplicator::papersMerged, this, [this](int count) {
            ToastWidget::showSuccess(QString("Merged %1 pairs").arg(count));
            if (!currentKeyword_.isEmpty()) {
                apiManager_->searchPapers(currentKeyword_, "", "", currentOffset_, currentLimit_);
            }
        });
        layout->addWidget(dedup);
        dlg->exec();
        dlg->deleteLater();
    });

    auto* similarityAction = toolsMenu->addAction("Paper &Similarity");
    connect(similarityAction, &QAction::triggered, this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Paper Similarity Analysis");
        dlg->resize(800, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperSimilarityWidget();
        if (resultView_) {
            QList<QPair<int, QString>> papers;
            for (const auto& p : resultView_->getPapers())
                papers.append({p.id, p.title + " " + p.abstractText});
            w->setPapers(papers);
        }
        connect(w, &PaperSimilarityWidget::pairClicked, this, [](int a, int b) {
            ToastWidget::showInfo(QString("Similar pair: #%1 ↔ #%2").arg(a).arg(b));
        });
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });

    auto* exportTplAction = toolsMenu->addAction("Export &Templates");
    connect(exportTplAction, &QAction::triggered, this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Export Template Manager");
        dlg->resize(900, 550);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new ExportTemplateManager();
        connect(w, &ExportTemplateManager::templateSelected, this, [](const ExportTemplate& t) {
            ToastWidget::showInfo(QString("Template: %1 (%2)").arg(t.name, t.format));
        });
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });

    auto* macroAction = toolsMenu->addAction("Keyboard &Macros");
    connect(macroAction, &QAction::triggered, this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Keyboard Macros");
        dlg->resize(600, 450);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new KeyboardMacroWidget();
        connect(w, &KeyboardMacroWidget::macroPlayed, this, [](int id) {
            ToastWidget::showSuccess(QString("Macro #%1 played").arg(id));
        });
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });

    auto* pluginAction = toolsMenu->addAction("&Plugin Manager");
    connect(pluginAction, &QAction::triggered, this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Plugin Manager");
        dlg->resize(800, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PluginLoaderWidget();
        w->scanPlugins(QCoreApplication::applicationDirPath() + "/plugins");
        connect(w, &PluginLoaderWidget::pluginLoaded, this, [](const QString& name) {
            ToastWidget::showSuccess(QString("Plugin loaded: %1").arg(name));
        });
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });

    auto* galleryAction = toolsMenu->addAction("Widget &Gallery");
    connect(galleryAction, &QAction::triggered, this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Widget Gallery");
        dlg->resize(700, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new WidgetGallery();
        w->addWidget("Analysis", "Similarity", "Jaccard / Cosine pair analysis");
        w->addWidget("Analysis", "Clustering", "K-means paper grouping");
        w->addWidget("Export", "CSV Template", "Standard CSV export format");
        w->addWidget("Export", "BibTeX Template", "BibTeX citation format");
        w->addWidget("Tools", "Macro Recorder", "Record and replay keyboard macros");
        w->addWidget("Tools", "Plugin Manager", "Load and manage plugins");
        connect(w, &WidgetGallery::widgetSelected, this, [](const QString& cat, const QString& name) {
            ToastWidget::showInfo(QString("Selected: %1 / %2").arg(cat, name));
        });
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });

    auto* mindmapAction = toolsMenu->addAction("Paper Mind &Map");
    connect(mindmapAction, &QAction::triggered, this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Paper Mind Map");
        dlg->resize(900, 600);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperMindMapWidget();
        if (resultView_) {
            QList<QPair<int, QString>> papers;
            for (const auto& p : resultView_->getPapers())
                papers.append({p.id, p.title});
            w->setPapers(papers);
        }
        connect(w, &PaperMindMapWidget::nodeDoubleClicked, this, [this](int paperId) {
            onPaperSelected(paperId);
        });
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });

    auto* batchDlAction = toolsMenu->addAction("Batch &Download");
    connect(batchDlAction, &QAction::triggered, this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Batch Download");
        dlg->resize(800, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new BatchDownloadWidget();
        if (resultView_) {
            QList<QPair<QString, QString>> tasks;
            for (const auto& p : resultView_->getPapers())
                tasks.append({p.title, p.pdfUrl});
            w->addTasks(tasks);
        }
        connect(w, &BatchDownloadWidget::downloadCompleted, this, [](int id, const QString& path) {
            ToastWidget::showSuccess(QString("Downloaded #%1 → %2").arg(id).arg(path));
        });
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });

    auto* timerAction = toolsMenu->addAction("Reading &Timer");
    connect(timerAction, &QAction::triggered, this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Reading Timer (Pomodoro)");
        dlg->resize(400, 550);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new ReadingTimerWidget();
        connect(w, &ReadingTimerWidget::timerCompleted, this, [](int secs) {
            ToastWidget::showSuccess(QString("Reading session done: %1 min").arg(secs / 60));
        });
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });

    auto* graderAction = toolsMenu->addAction("Paper &Grader");
    connect(graderAction, &QAction::triggered, this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Paper Grader");
        dlg->resize(900, 550);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperGraderWidget();
        connect(w, &PaperGraderWidget::gradeSaved, this, [](int id, double score) {
            ToastWidget::showSuccess(QString("Grade saved: #%1 → %2").arg(id).arg(score, 0, 'f', 1));
        });
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });

    auto* datavizAction = toolsMenu->addAction("Data &Visualization");
    connect(datavizAction, &QAction::triggered, this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Data Visualization");
        dlg->resize(900, 550);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new DataVisualizationWidget();
        if (resultView_) {
            QMap<QString, double> yearData;
            for (const auto& p : resultView_->getPapers()) {
                QString year = QString::number(p.year);
                yearData[year]++;
            }
            w->setBarData(yearData);
            w->setTitle("Papers by Year");
        }
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });

    auto* quizAction = toolsMenu->addAction("Paper &Quiz");
    connect(quizAction, &QAction::triggered, this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Paper Quiz / Flashcards");
        dlg->resize(900, 550);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperQuizWidget();
        connect(w, &PaperQuizWidget::quizCompleted, this, [](int total, int correct, int wrong) {
            ToastWidget::showSuccess(QString("Quiz done: %1/%2 correct").arg(correct).arg(total));
        });
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });

    auto* clipboardAction = toolsMenu->addAction("&Clipboard History");
    connect(clipboardAction, &QAction::triggered, this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Clipboard History");
        dlg->resize(700, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new ClipboardHistoryWidget();
        w->startMonitoring();
        connect(w, &ClipboardHistoryWidget::entryPasted, this, [](int id) {
            ToastWidget::showSuccess(QString("Entry #%1 pasted").arg(id));
        });
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });

    auto* translatorAction = toolsMenu->addAction("&Translator");
    connect(translatorAction, &QAction::triggered, this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Paper Translator");
        dlg->resize(800, 550);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperTranslatorWidget();
        connect(w, &PaperTranslatorWidget::translationCompleted, this, [](const QString&, const QString& result) {
            ToastWidget::showSuccess(QString("Translation: %1 chars").arg(result.length()));
        });
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });

    auto* biblioAction = toolsMenu->addAction("&Bibliography Builder");
    connect(biblioAction, &QAction::triggered, this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Bibliography Builder");
        dlg->resize(900, 550);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new BibliographyBuilderWidget();
        if (resultView_) {
            for (const auto& p : resultView_->getPapers()) {
                BibPaper bp;
                bp.id = p.id;
                bp.title = p.title;
                bp.authors = p.authors.split(QRegularExpression("[,;]"), Qt::SkipEmptyParts);
                bp.year = p.year;
                bp.journal = p.journal;
                bp.doi = p.doi;
                bp.abstractText = p.abstractText;
                w->addPaper(bp);
            }
        }
        connect(w, &BibliographyBuilderWidget::bibliographyGenerated, this, [](const QString& fmt, int count) {
            ToastWidget::showSuccess(QString("Generated %1 entries in %2").arg(count).arg(fmt));
        });
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });

    auto* langdetectAction = toolsMenu->addAction("&Language Detector");
    connect(langdetectAction, &QAction::triggered, this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Language Detector");
        dlg->resize(700, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new LanguageDetectorWidget();
        if (resultView_) {
            QStringList texts;
            for (const auto& p : resultView_->getPapers())
                texts << p.title + "\n" + p.abstractText;
            w->setBatchTexts(texts);
        }
        connect(w, &LanguageDetectorWidget::detectionCompleted, this, [](const LanguageResult& r) {
            ToastWidget::showInfo(QString("Detected: %1 (%2%)").arg(r.language).arg(r.confidence * 100, 0, 'f', 0));
        });
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });

    auto* storyboardAction = toolsMenu->addAction("&Storyboard");
    connect(storyboardAction, &QAction::triggered, this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Paper Storyboard");
        dlg->resize(900, 550);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperStoryboardWidget();
        if (resultView_) {
            QList<QPair<int, QString>> papers;
            for (const auto& p : resultView_->getPapers())
                papers.append({p.id, p.title});
            w->addPapers(papers);
        }
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });

    auto* mdEditorAction = toolsMenu->addAction("&Markdown Editor");
    connect(mdEditorAction, &QAction::triggered, this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Markdown Editor");
        dlg->resize(900, 550);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new MarkdownPreviewWidget();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });

    auto* queryBuilderAction = toolsMenu->addAction("Search &Query Builder");
    connect(queryBuilderAction, &QAction::triggered, this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Search Query Builder");
        dlg->resize(800, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new SearchQueryBuilder();
        connect(w, &SearchQueryBuilder::searchRequested, this, [this](const QString& query) {
            onSearch(query);
            ToastWidget::showSuccess("Query executed");
        });
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });

    auto* reportAction = toolsMenu->addAction("&Report Generator");
    connect(reportAction, &QAction::triggered, this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Paper Report Generator");
        dlg->resize(900, 600);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperReportGenerator();
        if (resultView_) {
            for (const auto& p : resultView_->getPapers()) {
                ReportPaper rp;
                rp.id = p.id;
                rp.title = p.title;
                rp.authors = p.authors.split(QRegularExpression("[,;]"), Qt::SkipEmptyParts);
                rp.year = p.year;
                rp.journal = p.journal;
                rp.abstractText = p.abstractText;
                rp.doi = p.doi;
                w->addPaper(rp);
            }
        }
        connect(w, &PaperReportGenerator::reportGenerated, this, [](const QString& fmt, int count) {
            ToastWidget::showSuccess(QString("Report: %1 papers, %2 format").arg(count).arg(fmt));
        });
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });

    auto* networkAction = toolsMenu->addAction("Author &Network");
    connect(networkAction, &QAction::triggered, this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Author Co-authorship Network");
        dlg->resize(800, 550);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperNetworkGraph();
        if (resultView_) {
            QList<QPair<int, QStringList>> papers;
            for (const auto& p : resultView_->getPapers())
                papers.append({p.id, p.authors.split(QRegularExpression("[,;]"), Qt::SkipEmptyParts)});
            w->buildFromPapers(papers);
        }
        connect(w, &PaperNetworkGraph::nodeClicked, this, [](const QString& name, const QString&) {
            ToastWidget::showInfo(QString("Author: %1").arg(name));
        });
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });

    auto* checklistAction = toolsMenu->addAction("Reading &Checklist");
    connect(checklistAction, &QAction::triggered, this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Reading Checklist");
        dlg->resize(700, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperChecklistWidget();
        if (resultView_ && !resultView_->getPapers().isEmpty()) {
            const auto& p = resultView_->getPapers().first();
            w->setPaper(p.id, p.title);
            w->addDefaultItems();
        }
        connect(w, &PaperChecklistWidget::checklistCompleted, this, [](int id) {
            ToastWidget::showSuccess(QString("Checklist #%1 completed!").arg(id));
        });
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });

    auto* sessionStatsAction = toolsMenu->addAction("Session &Statistics");
    connect(sessionStatsAction, &QAction::triggered, this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Session Statistics");
        dlg->resize(750, 550);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new SessionStatisticsWidget();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });

    auto* colorSchemeAction = toolsMenu->addAction("&Color Scheme");
    connect(colorSchemeAction, &QAction::triggered, this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Color Scheme Editor");
        dlg->resize(850, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new ColorSchemeEditor();
        connect(w, &ColorSchemeEditor::schemeApplied, this, [](const QString& name) {
            ToastWidget::showSuccess(QString("Applied scheme: %1").arg(name));
        });
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });

    auto* mergerAction = toolsMenu->addAction("Paper &Merger");
    connect(mergerAction, &QAction::triggered, this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Paper Merger");
        dlg->resize(900, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperMergerWidget();
        if (resultView_ && resultView_->getPapers().size() >= 2) {
            const auto& pA = resultView_->getPapers()[0];
            const auto& pB = resultView_->getPapers()[1];
            MergePaper mA{pA.id, pA.title, pA.authors, pA.year, pA.journal, pA.doi, pA.abstractText, "", pA.pdfUrl, 0};
            MergePaper mB{pB.id, pB.title, pB.authors, pB.year, pB.journal, pB.doi, pB.abstractText, "", pB.pdfUrl, 0};
            w->setPapers(mA, mB);
        }
        connect(w, &PaperMergerWidget::mergeCompleted, this, [this](int keepId, int removeId) {
            apiManager_->deletePaper(removeId);
            ToastWidget::showSuccess(QString("Merged: keep #%1, remove #%2").arg(keepId).arg(removeId));
        });
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });

    auto* summaryAction = toolsMenu->addAction("Abstract &Summarizer");
    connect(summaryAction, &QAction::triggered, this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Abstract Summarizer");
        dlg->resize(850, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new AbstractSummaryWidget();
        if (resultView_ && !resultView_->getPapers().isEmpty()) {
            QStringList abstracts;
            for (const auto& p : resultView_->getPapers()) {
                if (!p.abstractText.isEmpty()) abstracts << p.abstractText;
            }
            if (!abstracts.isEmpty()) w->setAbstract(abstracts.join("\n\n"));
        }
        connect(w, &AbstractSummaryWidget::extractionCompleted, this, [](const ExtractedInfo& info) {
            ToastWidget::showInfo(QString("Domain: %1 | %2 keywords").arg(info.domain).arg(info.keywords.size()));
        });
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });

    auto* depAction = toolsMenu->addAction("Paper &Dependencies");
    connect(depAction, &QAction::triggered, this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Paper Dependency Tracker");
        dlg->resize(800, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperDependencyWidget();
        if (resultView_) {
            for (const auto& p : resultView_->getPapers())
                w->addPaper(p.id, p.title);
        }
        connect(w, &PaperDependencyWidget::chainSelected, this, [](int id, const QList<int>& chain) {
            ToastWidget::showInfo(QString("Chain from #%1: %2 papers deep").arg(id).arg(chain.size()));
        });
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });

    auto* bmAction = toolsMenu->addAction("PDF &Bookmarks");
    connect(bmAction, &QAction::triggered, this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("PDF Bookmarks");
        dlg->resize(600, 450);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PdfBookmarkWidget();
        connect(w, &PdfBookmarkWidget::pageRequested, this, [](int page) {
            ToastWidget::showInfo(QString("Jump to page %1").arg(page));
        });
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });

    auto* compareSliderAction = toolsMenu->addAction("Paper &Comparison");
    connect(compareSliderAction, &QAction::triggered, this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Paper Comparison");
        dlg->resize(900, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperComparisonSlider();
        if (resultView_) {
            for (const auto& p : resultView_->getPapers()) {
                ComparePaper cp{p.id, p.title, p.authors, p.year, p.journal, p.doi, p.abstractText, "", 0, 0.0, 0, "", ""};
                w->addPaper(cp);
            }
        }
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });

    auto* notifRuleAction = toolsMenu->addAction("Notification &Rules");
    connect(notifRuleAction, &QAction::triggered, this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Notification Rule Editor");
        dlg->resize(900, 450);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new NotificationRuleEditor();
        connect(w, &NotificationRuleEditor::ruleTriggered, this, [](int id, const QString& action) {
            ToastWidget::showInfo(QString("Rule #%1 triggered: %2").arg(id).arg(action));
        });
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });

    auto* timelineAction = toolsMenu->addAction("Paper &Timeline");
    connect(timelineAction, &QAction::triggered, this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Paper Timeline Builder");
        dlg->resize(700, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperTimelineBuilder();
        if (resultView_) {
            for (const auto& p : resultView_->getPapers()) {
                TimelineEntry e;
                e.paperId = p.id;
                e.title = p.title;
                e.year = p.year;
                e.category = p.journal.isEmpty() ? "General" : p.journal.left(15);
                w->addEntry(e);
            }
        }
        connect(w, &PaperTimelineBuilder::entryClicked, this, [this](int id) {
            onPaperSelected(id);
        });
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });

    auto* commentAction = toolsMenu->addAction("Paper &Comments");
    connect(commentAction, &QAction::triggered, this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Paper Comments");
        dlg->resize(700, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperCommentWidget();
        if (resultView_) {
            for (const auto& p : resultView_->getPapers()) {
                (void)p;
            }
        }
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });

    auto* shareAction = toolsMenu->addAction("Paper &Share");
    connect(shareAction, &QAction::triggered, this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Paper Share");
        dlg->resize(600, 450);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperShareWidget();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });

    auto* embedAction = toolsMenu->addAction("Paper &Embeddings");
    connect(embedAction, &QAction::triggered, this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Paper Embedding Visualization");
        dlg->resize(800, 600);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperEmbeddingWidget();
        if (resultView_) {
            QList<QPair<int, QString>> papers;
            for (const auto& p : resultView_->getPapers()) {
                papers.append({p.id, p.title});
            }
            w->setPapers(papers);
            w->computeRandomEmbeddings();
        }
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });

    auto* schedulerAction = toolsMenu->addAction("Reading &Scheduler");
    connect(schedulerAction, &QAction::triggered, this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Reading Scheduler");
        dlg->resize(700, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new ReadingSchedulerWidget();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });

    auto* templateLibAction = toolsMenu->addAction("Paper &Templates");
    connect(templateLibAction, &QAction::triggered, this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Paper Template Library");
        dlg->resize(800, 600);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperTemplateLibrary();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });

    auto* annotAction = toolsMenu->addAction("Paper &Annotations");
    connect(annotAction, &QAction::triggered, this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Paper Annotation Highlighter");
        dlg->resize(700, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperAnnotationHighlighter();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });

    auto* citeNetAction = toolsMenu->addAction("Citation &Network");
    connect(citeNetAction, &QAction::triggered, this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Citation Network Visualizer");
        dlg->resize(850, 650);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new CitationNetworkVisualizer();
        if (resultView_) {
            QList<QPair<int, QString>> papers;
            for (const auto& p : resultView_->getPapers()) {
                papers.append({p.id, p.title});
            }
            w->setPapers(papers);
        }
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });

    auto* bmarkSyncAction = toolsMenu->addAction("Bookmark &Sync");
    connect(bmarkSyncAction, &QAction::triggered, this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Paper Bookmark Sync");
        dlg->resize(650, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperBookmarkSync();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });

    auto* progressAction = toolsMenu->addAction("Reading &Progress");
    connect(progressAction, &QAction::triggered, this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Reading Progress Dashboard");
        dlg->resize(700, 550);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new ReadingProgressDashboard();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });

    auto* ratingAction = toolsMenu->addAction("Paper &Ratings");
    connect(ratingAction, &QAction::triggered, this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Paper Rating Chart");
        dlg->resize(700, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperRatingChart();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });

    auto* workflowAction = toolsMenu->addAction("&Workflow Automator");
    connect(workflowAction, &QAction::triggered, this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Paper Workflow Automator");
        dlg->resize(800, 550);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperWorkflowAutomator();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });

    auto* citeGraphAction = toolsMenu->addAction("Citation &Graph Explorer");
    connect(citeGraphAction, &QAction::triggered, this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Citation Graph Explorer");
        dlg->resize(900, 650);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new CitationGraphExplorer();
        if (resultView_) {
            QList<QPair<int, QString>> papers;
            for (const auto& p : resultView_->getPapers()) {
                papers.append({p.id, p.title});
            }
            w->loadFromPapers(papers);
        }
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });

    auto* insightAction = toolsMenu->addAction("Paper &Insights");
    connect(insightAction, &QAction::triggered, this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Paper Insight Extractor");
        dlg->resize(850, 550);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperInsightExtractor();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });

    auto* journalAction = toolsMenu->addAction("Reading &Journal");
    connect(journalAction, &QAction::triggered, this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Reading Journal");
        dlg->resize(800, 550);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new ReadingJournalWidget();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });

    auto* crossRefAction = toolsMenu->addAction("Cross-&References");
    connect(crossRefAction, &QAction::triggered, this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Paper Cross-Reference");
        dlg->resize(800, 550);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperCrossReference();
        if (resultView_) {
            QList<QPair<int, QString>> papers;
            for (const auto& p : resultView_->getPapers()) {
                papers.append({p.id, p.title});
            }
            w->setPapers(papers);
        }
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });

    auto* peerReviewAction = toolsMenu->addAction("Peer &Review");
    connect(peerReviewAction, &QAction::triggered, this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Paper Peer Review");
        dlg->resize(800, 550);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperPeerReviewer();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });

    auto* matrixAction = toolsMenu->addAction("Literature &Matrix");
    connect(matrixAction, &QAction::triggered, this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Literature Matrix");
        dlg->resize(800, 550);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new LiteratureMatrixWidget();
        if (resultView_) {
            QList<QPair<int, QString>> papers;
            for (const auto& p : resultView_->getPapers()) {
                papers.append({p.id, p.title});
            }
            w->setPapers(papers);
            w->autoFill();
        }
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });

    auto* conceptAction = toolsMenu->addAction("Paper &Concepts");
    connect(conceptAction, &QAction::triggered, this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Paper Concept Map");
        dlg->resize(850, 600);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperConceptMap();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });

    auto* streakAction = toolsMenu->addAction("Reading &Streak");
    connect(streakAction, &QAction::triggered, this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Reading Streak Tracker");
        dlg->resize(600, 420);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new ReadingStreakTracker();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });

    auto* batchExportAction = toolsMenu->addAction("Batch &Export");
    connect(batchExportAction, &QAction::triggered, this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Batch Export");
        dlg->resize(850, 550);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperExportBatch();
        if (resultView_) {
            QList<QPair<int, QString>> papers;
            for (const auto& p : resultView_->getPapers()) {
                papers.append({p.id, p.title});
            }
            w->setPapers(papers);
        }
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });

    auto* sumChainAction = toolsMenu->addAction("Summarizer &Chain");
    connect(sumChainAction, &QAction::triggered, this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Paper Summarizer Chain");
        dlg->resize(900, 550);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperSummarizerChain();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });

    auto* trendAction = toolsMenu->addAction("Research &Trends");
    connect(trendAction, &QAction::triggered, this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Research Trend Analyzer");
        dlg->resize(700, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new ResearchTrendAnalyzer();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });

    auto* dupAction = toolsMenu->addAction("&Duplicate Detector");
    connect(dupAction, &QAction::triggered, this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Paper Duplicate Detector");
        dlg->resize(800, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperDuplicateDetector();
        if (resultView_) {
            QList<QPair<int, QString>> papers;
            for (const auto& p : resultView_->getPapers()) {
                papers.append({p.id, p.title});
            }
            w->setPapers(papers);
        }
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });

    auto* goalAction = toolsMenu->addAction("Reading &Goals");
    connect(goalAction, &QAction::triggered, this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Reading Goal Tracker");
        dlg->resize(600, 450);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new ReadingGoalTracker();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });

    auto* kbAction = toolsMenu->addAction("Knowledge &Base");
    connect(kbAction, &QAction::triggered, this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Paper Knowledge Base");
        dlg->resize(850, 550);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperKnowledgeBase();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });

    auto* refExtractAction = toolsMenu->addAction("Reference &Extractor");
    connect(refExtractAction, &QAction::triggered, this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Paper Reference Extractor");
        dlg->resize(900, 550);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperReferenceExtractor();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });

    auto* searchHistAction = toolsMenu->addAction("Search &History Analysis");
    connect(searchHistAction, &QAction::triggered, this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Search History Analyzer");
        dlg->resize(650, 450);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new SearchHistoryAnalyzer();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });

    auto* citeStyleAction = toolsMenu->addAction("Citation &Style Editor");
    connect(citeStyleAction, &QAction::triggered, this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Citation Style Editor");
        dlg->resize(800, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperCitationStyleEditor();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });

    auto* sessionLogAction = toolsMenu->addAction("Session &Log");
    connect(sessionLogAction, &QAction::triggered, this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Reading Session Log");
        dlg->resize(700, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new ReadingSessionLog();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });

    auto* figureAction = toolsMenu->addAction("&Figure Extractor");
    connect(figureAction, &QAction::triggered, this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Paper Figure Extractor");
        dlg->resize(750, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperFigureExtractor();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });

    auto* topicModelerAction = toolsMenu->addAction("&Topic Modeler");
    connect(topicModelerAction, &QAction::triggered, this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Paper Topic Modeler");
        dlg->resize(750, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperTopicModeler();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });

    auto* speedAnalyzerAction = toolsMenu->addAction("&Reading Speed Analyzer");
    connect(speedAnalyzerAction, &QAction::triggered, this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Reading Speed Analyzer");
        dlg->resize(750, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new ReadingSpeedAnalyzer();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });

    auto* citationCounterAction = toolsMenu->addAction("&Citation Counter");
    connect(citationCounterAction, &QAction::triggered, this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Paper Citation Counter");
        dlg->resize(750, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperCitationCounter();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });

    auto* keywordExtractorAction = toolsMenu->addAction("&Keyword Extractor");
    connect(keywordExtractorAction, &QAction::triggered, this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Paper Keyword Extractor");
        dlg->resize(750, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperKeywordExtractor();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });

    auto* readingPlanAction = toolsMenu->addAction("&Reading Plan");
    connect(readingPlanAction, &QAction::triggered, this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Reading Plan");
        dlg->resize(750, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new ReadingPlanWidget();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });

    auto* collabBoardAction = toolsMenu->addAction("&Collaboration Board");
    connect(collabBoardAction, &QAction::triggered, this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Paper Collaboration Board");
        dlg->resize(750, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperCollaborationBoard();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });

    auto* researchNoteAction = toolsMenu->addAction("&Research Notes");
    connect(researchNoteAction, &QAction::triggered, this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Research Notes");
        dlg->resize(750, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new ResearchNoteWidget();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });

    auto* feedbackCollectorAction = toolsMenu->addAction("&Feedback Collector");
    connect(feedbackCollectorAction, &QAction::triggered, this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Paper Feedback Collector");
        dlg->resize(750, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperFeedbackCollector();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });

    auto* litTimelineAction = toolsMenu->addAction("&Literature Timeline");
    connect(litTimelineAction, &QAction::triggered, this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Literature Timeline");
        dlg->resize(750, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new LiteratureTimelineWidget();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });

    auto* tagCloudAction = toolsMenu->addAction("&Tag Cloud");
    connect(tagCloudAction, &QAction::triggered, this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Paper Tag Cloud");
        dlg->resize(750, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperTagCloudWidget();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });

    auto* heatmapAction = toolsMenu->addAction("&Reading Heatmap");
    connect(heatmapAction, &QAction::triggered, this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Paper Reading Heatmap");
        dlg->resize(750, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperReadingHeatmap();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });

    auto* citationStyleAction = toolsMenu->addAction("&Citation Style Generator");
    connect(citationStyleAction, &QAction::triggered, this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Citation Style Generator");
        dlg->resize(750, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new CitationStyleGenerator();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });

    auto* versionDiffAction = toolsMenu->addAction("&Version Diff");
    connect(versionDiffAction, &QAction::triggered, this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Paper Version Diff");
        dlg->resize(750, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperVersionDiff();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });

    auto* researchCalendarAction = toolsMenu->addAction("&Research Calendar");
    connect(researchCalendarAction, &QAction::triggered, this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Research Calendar");
        dlg->resize(750, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new ResearchCalendarWidget();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });

    auto* sentimentAction = toolsMenu->addAction("&Sentiment Analyzer");
    connect(sentimentAction, &QAction::triggered, this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Paper Sentiment Analyzer");
        dlg->resize(750, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperSentimentAnalyzer();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });

    auto* citationNetworkAction = toolsMenu->addAction("&Citation Network");
    connect(citationNetworkAction, &QAction::triggered, this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Paper Citation Network");
        dlg->resize(750, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperCitationNetwork();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });

    auto* coReadingAction = toolsMenu->addAction("&Co-Reading Tracker");
    connect(coReadingAction, &QAction::triggered, this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Paper Co-Reading Tracker");
        dlg->resize(750, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperCoReadingTracker();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });

    auto* qaChatAction = toolsMenu->addAction("&Paper QA Chat");
    connect(qaChatAction, &QAction::triggered, this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Paper QA Chat");
        dlg->resize(750, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperQaChatWidget();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });

    auto* trendHeatmapAction = toolsMenu->addAction("&Trend Heatmap");
    connect(trendHeatmapAction, &QAction::triggered, this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Paper Trend Heatmap");
        dlg->resize(750, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperTrendHeatmap();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });

    auto* autoTaggerAction = toolsMenu->addAction("&Auto Tagger");
    connect(autoTaggerAction, &QAction::triggered, this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Paper Auto Tagger");
        dlg->resize(750, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperAutoTagger();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });

    toolsMenu->addAction("Reading Score", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Reading Score");
        dlg->setMinimumSize(560, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperReadingScoreWidget();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });

    toolsMenu->addAction("Math Formula Renderer", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Math Formula Renderer");
        dlg->setMinimumSize(560, 480);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperMathFormulaRenderer();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });

    toolsMenu->addAction("Reading Groups", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Reading Groups");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperReadingGroupWidget();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });

    toolsMenu->addAction("Alert Monitor", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Alert Monitor");
        dlg->setMinimumSize(600, 480);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperAlertMonitor();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });

    toolsMenu->addAction("Geography Map", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Geography Map");
        dlg->setMinimumSize(620, 520);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperGeographyMap();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });

    toolsMenu->addAction("Citation Radar Chart", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Citation Radar Chart");
        dlg->setMinimumSize(620, 520);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperCitationRadarChart();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });

    toolsMenu->addAction("Research Timeline", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Research Timeline");
        dlg->setMinimumSize(620, 520);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperResearchTimeline();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });

    toolsMenu->addAction("Argument Parser", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Argument Parser");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperArgumentParser();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });

    toolsMenu->addAction("Code Snippet Manager", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Code Snippet Manager");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperCodeSnippetManager();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });

    toolsMenu->addAction("Resource Allocator", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Resource Allocator");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperResourceAllocator();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });

    toolsMenu->addAction("Influence Graph", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Influence Graph");
        dlg->setMinimumSize(620, 520);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperInfluenceGraph();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });

    toolsMenu->addAction("Methodology Extractor", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Methodology Extractor");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperMethodologyExtractor();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });

    toolsMenu->addAction("Data Transform", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Data Transform");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperDataTransformWidget();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });

    toolsMenu->addAction("Experiment Tracker", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Experiment Tracker");
        dlg->setMinimumSize(620, 520);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperExperimentTracker();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });

    toolsMenu->addAction("Reading Notes Organizer", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Reading Notes Organizer");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperReadingNotesOrganizer();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });

    toolsMenu->addAction("Collaboration Heatmap", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Collaboration Heatmap");
        dlg->setMinimumSize(600, 520);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperCollaborationHeatmap();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });

    toolsMenu->addAction("Bias Detector", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Bias Detector");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperBiasDetector();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });

    toolsMenu->addAction("Paper Validation", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Paper Validation");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperValidationWidget();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });

    toolsMenu->addAction("Grant Tracker", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Grant Tracker");
        dlg->setMinimumSize(620, 520);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperGrantTracker();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });

    toolsMenu->addAction("Reading Challenges", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Reading Challenges");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperReadingChallengeWidget();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });

    toolsMenu->addAction("Citation Tree Map", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Citation Tree Map");
        dlg->setMinimumSize(620, 520);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperCitationTreeMap();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });

    toolsMenu->addAction("Reproducibility Checker", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Reproducibility Checker");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperReproducibilityChecker();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });

    toolsMenu->addAction("Pomodoro Timer", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Pomodoro Timer");
        dlg->setMinimumSize(580, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperPomodoroTimer();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });

    toolsMenu->addAction("Dataset Manager", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Dataset Manager");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperDatasetManager();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });

    toolsMenu->addAction("Reading Analytics", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Reading Analytics");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperReadingAnalyticsWidget();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });

    toolsMenu->addAction("Workflow Sankey", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Workflow Sankey");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperWorkflowSankey();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });

    toolsMenu->addAction("Entity Extractor", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Entity Extractor");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperEntityExtractor();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });

    toolsMenu->addAction("Side-by-Side Compare", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Side-by-Side Compare");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperSideBySideCompare();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });

    toolsMenu->addAction("Conference Planner", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Conference Planner");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperConferencePlanner();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });

    toolsMenu->addAction("Peer Review Tracker", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Peer Review Tracker");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperPeerReviewTracker();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });

    toolsMenu->addAction("Survey Builder", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Survey Builder");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperSurveyBuilder();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });

    toolsMenu->addAction("Citation Context Analyzer", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Citation Context Analyzer");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperCitationContextAnalyzer();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });

    toolsMenu->addAction("Reading Path Optimizer", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Reading Path Optimizer");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperReadingPathOptimizer();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });

    toolsMenu->addAction("Knowledge Graph Explorer", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Knowledge Graph Explorer");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperKnowledgeGraphExplorer();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });

    toolsMenu->addAction("Author Disambiguator", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Author Disambiguator");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperAuthorDisambiguator();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });

    toolsMenu->addAction("Citation Timing Analyzer", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Citation Timing Analyzer");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperCitationTimingAnalyzer();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });

    toolsMenu->addAction("Reading List Sorter", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Reading List Sorter");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperReadingListSorter();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });

    toolsMenu->addAction("Trend Predictor", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Trend Predictor");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperTrendPredictor();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });

    toolsMenu->addAction("Collaboration Matcher", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Collaboration Matcher");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperCollaborationMatcher();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });

    toolsMenu->addAction("Formula Search Engine", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Formula Search Engine");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperFormulaSearchEngine();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });

    toolsMenu->addAction("Recommender Feedback", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Recommender Feedback");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperRecommenderFeedback();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });

    toolsMenu->addAction("Reading Speed Tracker", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Reading Speed Tracker");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperReadingSpeedTracker();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });

    toolsMenu->addAction("Citation Network D3", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Citation Network D3");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperCitationNetworkD3();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });

    toolsMenu->addAction("Project Timeline", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Project Timeline");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperProjectTimelineWidget();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });

    toolsMenu->addAction("LaTeX Symbol Finder", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("LaTeX Symbol Finder");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperLatexSymbolFinder();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });

    toolsMenu->addAction("Argument Strength Analyzer", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Argument Strength Analyzer");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperArgumentStrengthAnalyzer();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });

    toolsMenu->addAction("Reading Mood Tracker", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Reading Mood Tracker");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperReadingMoodTracker();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });

    toolsMenu->addAction("Citation Impact Cloud", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Citation Impact Cloud");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperCitationImpactCloud();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });

    toolsMenu->addAction("Budget Planner", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Budget Planner");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperBudgetPlanner();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });

    toolsMenu->addAction("Batch Renamer", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Batch Renamer");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperBatchRenamer();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Semantic Similarity Matrix", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Semantic Similarity Matrix");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperSemanticSimilarityMatrix();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Reading Retention Tracker", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Reading Retention Tracker");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperReadingRetentionTracker();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Citation Flow Diagram", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Citation Flow Diagram");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperCitationFlowDiagram();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Ethics Checker", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Ethics Checker");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperEthicsChecker();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Regex Search Tool", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Regex Search Tool");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperRegexSearchTool();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Contradiction Detector", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Contradiction Detector");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperContradictionDetector();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Annotation Sync", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Annotation Sync");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperAnnotationSync();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Research Heatmap", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Research Heatmap");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperResearchHeatmap();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Collaboration Score", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Collaboration Score");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperCollaborationScore();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Markdown Exporter", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Markdown Exporter");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperMarkdownExporter();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Claim Strength Analyzer", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Claim Strength Analyzer");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperClaimStrengthAnalyzer();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Reading Session Timer", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Reading Session Timer");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperReadingSessionTimer();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Keyword Evolution", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Keyword Evolution");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperKeywordEvolution();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Review Assignment", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Review Assignment");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperReviewAssignment();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Table Extractor", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Table Extractor");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperTableExtractor();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Methodology Comparator", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Methodology Comparator");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperMethodologyComparator();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Reading Speed Test", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Reading Speed Test");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperReadingSpeedTest();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Citation Sankey", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Citation Sankey");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperCitationSankey();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Project Milestone", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Project Milestone");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperProjectMilestone();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Figure Captioner", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Figure Captioner");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperFigureCaptioner();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Evidence Strength", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Evidence Strength");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperEvidenceStrength();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Vocabulary Tracker", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Vocabulary Tracker");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperVocabularyTracker();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Trend Sparkline", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Trend Sparkline");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperTrendSparkline();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Team Availability", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Team Availability");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperTeamAvailability();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("PDF Annotator", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("PDF Annotator");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperPDFAnnotator();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Novelty Scorer", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Novelty Scorer");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperNoveltyScorer();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Reading Goals", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Reading Goals");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperReadingGoalWidget();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Impact Treemap", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Impact Treemap");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperImpactTreemap();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Budget Forecast", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Budget Forecast");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperBudgetForecast();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Code Snippet Extractor", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Code Snippet Extractor");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperCodeSnippetExtractor();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Claim Verifier", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Claim Verifier");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperClaimVerifier();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Reading Milestone", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Reading Milestone");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperReadingMilestone();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Donut Chart", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Donut Chart");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperDonutChart();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Invoice Tracker", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Invoice Tracker");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperInvoiceTracker();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("API Tester", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("API Tester");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperApiTester();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Evidence Chain", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Evidence Chain");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperEvidenceChain();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Reading Challenge", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Reading Challenge");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperReadingChallenge();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Bubble Chart", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Bubble Chart");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperBubbleChart();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Expense Logger", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Expense Logger");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperExpenseLogger();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Web Scraper", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Web Scraper");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperWebScraper();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Hypothesis Tracker", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Hypothesis Tracker");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperHypothesisTracker();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Reading Puzzle", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Reading Puzzle");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperReadingPuzzle();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Radar Chart", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Radar Chart");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperRadarChart();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Contract Manager", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Contract Manager");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperContractManager();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Cron Scheduler", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Cron Scheduler");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperCronScheduler();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Argument Mapper", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Argument Mapper");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperArgumentMapper();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Reading Badge", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Reading Badge");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperReadingBadge();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Heatmap Grid", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Heatmap Grid");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperHeatmapGrid();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Proposal Tracker", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Proposal Tracker");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperProposalTracker();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Log Viewer", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Log Viewer");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperLogViewer();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Fallacy Detector", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Fallacy Detector");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperFallacyDetector();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Reading Streak Board", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Reading Streak Board");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperReadingStreakBoard();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Scatter Plot", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Scatter Plot");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperScatterPlot();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Vendor Rating", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Vendor Rating");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperVendorRating();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Config Editor", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Config Editor");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperConfigEditor();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Bias Checker", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Bias Checker");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperBiasChecker();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Reading Gamification", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Reading Gamification");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperReadingGamification();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Contour Plot", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Contour Plot");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperContourPlot();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("License Tracker", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("License Tracker");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperLicenseTracker();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Environment Switcher", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Environment Switcher");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperEnvironmentSwitcher();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Sentiment Tracker", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Sentiment Tracker");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperSentimentTracker();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Reading Achievement", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Reading Achievement");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperReadingAchievement();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Waterfall Chart", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Waterfall Chart");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperWaterfallChart();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Compliance Monitor", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Compliance Monitor");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperComplianceMonitor();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Log Analyzer", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Log Analyzer");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperLogAnalyzer();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Topic Clusterer", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Topic Clusterer");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperTopicClusterer();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Reading Dashboard", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Reading Dashboard");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperReadingDashboard();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Funnel Chart", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Funnel Chart");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperFunnelChart();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Audit Logger", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Audit Logger");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperAuditLogger();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Health Monitor", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Health Monitor");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperHealthMonitor();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Argument Strength", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Argument Strength");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperArgumentStrength();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Reading Scorecard", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Reading Scorecard");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperReadingScorecard();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Gauge Chart", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Gauge Chart");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperGaugeChart();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Policy Manager", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Policy Manager");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperPolicyManager();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Cache Inspector", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Cache Inspector");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperCacheInspector();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Concept Extractor", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Concept Extractor");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperConceptExtractor();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Reading Heatmap Grid", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Reading Heatmap Grid");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperReadingHeatmapGrid();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Sankey Diagram", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Sankey Diagram");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperSankeyDiagram();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Risk Assessor", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Risk Assessor");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperRiskAssessor();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Feature Flag Manager", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Feature Flag Manager");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperFeatureFlagManager();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Entity Recognizer", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Entity Recognizer");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperEntityRecognizer();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Reading Progress Grid", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Reading Progress Grid");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperReadingProgressGrid();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Treemap Chart", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Treemap Chart");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperTreemapChart();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Cost Estimator", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Cost Estimator");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperCostEstimator();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Plugin Installer", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Plugin Installer");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperPluginInstaller();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Argument Visualizer", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Argument Visualizer");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperArgumentVisualizer();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Score Predictor", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Score Predictor");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperReadingScorePredictor();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Word Cloud", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Word Cloud");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperWordCloud();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Vendor Comparison", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Vendor Comparison");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperVendorComparison();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Deployment Tracker", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Deployment Tracker");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperDeploymentTracker();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Citation Analyzer", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Citation Analyzer");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperCitationAnalyzer();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Reading Cluster", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Reading Cluster");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperReadingCluster();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Heatmap Widget", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Heatmap Widget");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperHeatmapWidget();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Sprint Board", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Sprint Board");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperSprintBoard();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Keyboard Shortcut", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Keyboard Shortcut");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperKeyboardShortcut();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Claim Tracker", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Claim Tracker");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperClaimTracker();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Reading Calendar", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Reading Calendar");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperReadingCalendar();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Network Graph 3D", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Network Graph 3D");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperNetworkGraph3D();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Time Tracker", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Time Tracker");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperTimeTracker();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Theme Builder", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Theme Builder");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperThemeBuilder();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Sentiment Explorer", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Sentiment Explorer");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperSentimentExplorer();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Reading Competency", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Reading Competency");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperReadingCompetency();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Parallel Coordinates", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Parallel Coordinates");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperParallelCoordinates();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Resource Scheduler", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Resource Scheduler");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperResourceScheduler();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Script Runner", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Script Runner");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperScriptRunner();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Topic Evolution", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Topic Evolution");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperTopicEvolution();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Reading Flow", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Reading Flow");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperReadingFlow();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Violin Plot", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Violin Plot");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperViolinPlot();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Meeting Scheduler", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Meeting Scheduler");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperMeetingScheduler();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Context Menu Editor", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Context Menu Editor");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperContextMenuEditor();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Argument Miner", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Argument Miner");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperArgumentMiner();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Reading Workshop", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Reading Workshop");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperReadingWorkshop();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Radar Spinner", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Radar Spinner");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperRadarSpinner();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Inventory Tracker", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Inventory Tracker");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperInventoryTracker();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Notification Filter", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Notification Filter");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperNotificationFilter();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Evidence Evaluator", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Evidence Evaluator");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperEvidenceEvaluator();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Annotation Studio", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Annotation Studio");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperAnnotationStudio();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Flow Diagram", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Flow Diagram");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperFlowDiagram();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Funding Monitor", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Funding Monitor");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperFundingMonitor();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Update Checker", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Update Checker");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperUpdateChecker();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Bias Scanner", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Bias Scanner");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperBiasScanner();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Citation Logger", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Citation Logger");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperCitationLogger();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Bubble Matrix", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Bubble Matrix");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperBubbleMatrix();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Task Automator", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Task Automator");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperTaskAutomator();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Log Inspector", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Log Inspector");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperLogInspector();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Concept Gap", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Concept Gap");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperConceptGap();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Skill Assessor", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Skill Assessor");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperSkillAssessor();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Gantt Chart", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Gantt Chart");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperGanttChart();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Paper Recommender", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Paper Recommender");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperPaperRecommender();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Snippet Vault", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Snippet Vault");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperSnippetVault();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Novelty Score", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Novelty Score");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperNoveltyScore();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Study Planner", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Study Planner");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperStudyPlanner();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Dot Plot", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Dot Plot");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperDotPlot();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Lab Notebook", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Lab Notebook");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperLabNotebook();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Version Tree", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Version Tree");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperVersionTree();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Claim Validator", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Claim Validator");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperClaimValidator();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Reading Journal", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Reading Journal");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperReadingJournal();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Stacked Chart", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Stacked Chart");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperStackedChart();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Team Workspace", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Team Workspace");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperTeamWorkspace();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Markup Editor", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Markup Editor");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperMarkupEditor();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Cohort Analyzer", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Cohort Analyzer");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperCohortAnalyzer();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Flashcard Deck", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Flashcard Deck");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperFlashcardDeck();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Ring Chart", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Ring Chart");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperRingChart();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Milestone Tracker", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Milestone Tracker");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperMilestoneTracker();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Plugin Manager", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Plugin Manager");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperPluginManager();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Contradiction Finder", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Contradiction Finder");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperContradictionFinder();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Spaced Repetition", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Spaced Repetition");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperSpacedRepetition();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Area Chart", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Area Chart");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperAreaChart();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Risk Matrix", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Risk Matrix");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperRiskMatrix();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Macro Recorder", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Macro Recorder");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperMacroRecorder();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Hypothesis Tester", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Hypothesis Tester");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperHypothesisTester();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Vocabulary Builder", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Vocabulary Builder");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperVocabularyBuilder();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Candlestick Chart", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Candlestick Chart");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperCandlestickChart();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Budget Tracker", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Budget Tracker");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperBudgetTracker();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Color Picker", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Color Picker");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperColorPicker();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Correlation Engine", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Correlation Engine");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperCorrelationEngine();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Quiz Generator", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Quiz Generator");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperQuizGenerator();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Pyramid Chart", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Pyramid Chart");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperPyramidChart();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Team Scoreboard", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Team Scoreboard");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperTeamScoreboard();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Font Browser", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Font Browser");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperFontBrowser();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Factor Analyzer", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Factor Analyzer");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperFactorAnalyzer();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Note Organizer", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Note Organizer");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperNoteOrganizer();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Bullet Chart", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Bullet Chart");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperBulletChart();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Asset Manager", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Asset Manager");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperAssetManager();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Regex Tester", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Regex Tester");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperRegexTester();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Outlier Detector", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Outlier Detector");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperOutlierDetector();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Paper Timeline", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Paper Timeline");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperPaperTimeline();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Lollipop Chart", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Lollipop Chart");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperLollipopChart();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Conference Tracker", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Conference Tracker");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperConferenceTracker();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Diff Viewer", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Diff Viewer");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperDiffViewer();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Cluster Validator", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Cluster Validator");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperClusterValidator();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Reference Graph", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Reference Graph");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperReferenceGraph();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Histogram Widget", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Histogram Widget");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperHistogramWidget();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Experiment Logger", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Experiment Logger");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperExperimentLogger();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Archive Manager", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Archive Manager");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperArchiveManager();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Topic Tracker", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Topic Tracker");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperTopicTracker();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Reading Thermometer", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Reading Thermometer");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperReadingThermometer();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Step Chart", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Step Chart");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperStepChart();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Project Board", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Project Board");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperProjectBoard();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Clipboard Manager", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Clipboard Manager");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperClipboardManager();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Anomaly Detector", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Anomaly Detector");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperAnomalyDetector();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Reading Radar", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Reading Radar");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperReadingRadar();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Treemap Widget", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Treemap Widget");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperTreemapWidget();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Survey Manager", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Survey Manager");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperSurveyManager();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Bookmark Organizer", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Bookmark Organizer");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperBookmarkOrganizer();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Pattern Miner", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Pattern Miner");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperPatternMiner();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Reading Pulse", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Reading Pulse");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperReadingPulse();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Radial Chart", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Radial Chart");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperRadialChart();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Collaboration Hub", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Collaboration Hub");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperCollaborationHub();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Code Vault", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Code Vault");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperCodeVault();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Bias Monitor", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Bias Monitor");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperBiasMonitor();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Reading Vitals", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Reading Vitals");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperReadingVitals();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Waffle Chart", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Waffle Chart");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperWaffleChart();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Team Sync", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Team Sync");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperTeamSync();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Formula Editor", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Formula Editor");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperFormulaEditor();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Causal Finder", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Causal Finder");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperCausalFinder();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Reading Compass", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Reading Compass");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperReadingCompass();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Marimekko Chart", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Marimekko Chart");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperMarimekkoChart();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Resource Pool", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Resource Pool");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperResourcePool();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Unit Converter", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Unit Converter");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperUnitConverter();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Relevance Scorer", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Relevance Scorer");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperRelevanceScorer();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Reading Sprint", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Reading Sprint");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperReadingSprint();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Parallel Plot", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Parallel Plot");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperParallelPlot();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Version Tracker", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Version Tracker");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperVersionTracker();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Timer Widget", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Timer Widget");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperTimerWidget();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Semantic Mapper", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Semantic Mapper");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperSemanticMapper();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Reading Waves", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Reading Waves");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperReadingWaves();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Dendrogram View", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Dendrogram View");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperDendrogramView();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Workflow Board", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Workflow Board");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperWorkflowBoard();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Checksum Tool", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Checksum Tool");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperChecksumTool();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Embedding Visualizer", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Embedding Visualizer");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperEmbeddingVisualizer();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Reading Gradient", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Reading Gradient");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperReadingGradient();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Heatmap Matrix", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Heatmap Matrix");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperHeatmapMatrix();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Pipeline View", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Pipeline View");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperPipelineView();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Diff Tool", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Diff Tool");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperDiffTool();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Knowledge Extractor", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Knowledge Extractor");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperKnowledgeExtractor();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Reading Elevation", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Reading Elevation");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperReadingElevation();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Box Plot", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Box Plot");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperBoxPlotWidget();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Timeline Gantt", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Timeline Gantt");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperTimelineGantt();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("File Watcher", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("File Watcher");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperFileWatcher();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Novelty Score", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Novelty Score");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperNoveltyScore();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Study Planner", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Study Planner");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperStudyPlanner();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Dot Plot", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Dot Plot");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperDotPlot();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Lab Notebook", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Lab Notebook");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperLabNotebook();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Version Tree", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Version Tree");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperVersionTree();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Contrast Analyzer", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Contrast Analyzer");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperContrastAnalyzer();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Reading Goal Tree", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Reading Goal Tree");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperReadingGoalTree();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Lollipop Chart", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Lollipop Chart");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperLollipopChart();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Experiment Tracker", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Experiment Tracker");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperExperimentTracker();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Log Rotator", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Log Rotator");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperLogRotator();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Claim Verifier", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Claim Verifier");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperClaimVerifier();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Reading Heatmap", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Reading Heatmap");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperReadingHeatmap();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Range Chart", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Range Chart");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperRangeChart();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Protocol Manager", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Protocol Manager");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperProtocolManager();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Config Differ", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Config Differ");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperConfigDiffer();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Evidence Weigher", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Evidence Weigher");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperEvidenceWeigher();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Reading Depth", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Reading Depth");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperReadingDepth();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Bullet Chart", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Bullet Chart");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperBulletChart();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Hypothesis Board", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Hypothesis Board");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperHypothesisBoard();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Patch Manager", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Patch Manager");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperPatchManager();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Bias Detector", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Bias Detector");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperBiasDetector();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Reading Scorecard", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Reading Scorecard");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperReadingScorecard();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Funnel Chart", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Funnel Chart");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperFunnelChart();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Task Board", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Task Board");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperTaskBoard();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Token Counter", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Token Counter");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperTokenCounter();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Reproducibility Checker", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Reproducibility Checker");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperReproducibilityChecker();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Reading Pace", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Reading Pace");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperReadingPace();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Candlestick Chart", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Candlestick Chart");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperCandlestickChart();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Milestone Tracker", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Milestone Tracker");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperMilestoneTracker();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Service Monitor", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Service Monitor");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperServiceMonitor();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Argument Strength", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Argument Strength");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperArgumentStrength();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Reading Journal", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Reading Journal");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperReadingJournal();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Waterfall Chart", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Waterfall Chart");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperWaterfallChart();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Release Planner", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Release Planner");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperReleasePlanner();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Deprecation Tracker", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Deprecation Tracker");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperDeprecationTracker();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Citation Verifier", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Citation Verifier");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperCitationVerifier();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Reading Backlog", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Reading Backlog");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperReadingBacklog();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Radar Chart", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Radar Chart");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperRadarChart();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Sprint Board", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Sprint Board");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperSprintBoard();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Key Store", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Key Store");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperKeyStore();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Impact Predictor", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Impact Predictor");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperImpactPredictor();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Reading Velocity", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Reading Velocity");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperReadingVelocity();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Sankey Chart", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Sankey Chart");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperSankeyChart();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Retrospective", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Retrospective");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperRetrospective();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Secret Vault", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Secret Vault");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperSecretVault();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Trend Detector", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Trend Detector");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperTrendDetector();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Reading Binge", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Reading Binge");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperReadingBinge();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Tornado Chart", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Tornado Chart");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperTornadoChart();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Capacity Planner", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Capacity Planner");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperCapacityPlanner();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Feature Flag", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Feature Flag");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperFeatureFlag();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Citation Network", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Citation Network");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperCitationNetwork();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Reading Streak", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Reading Streak");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperReadingStreak();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Violin Chart", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Violin Chart");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperViolinChart();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Dependency Graph", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Dependency Graph");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperDependencyGraph();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Audit Log", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Audit Log");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperAuditLog();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Semantics Mapper", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Semantics Mapper");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperSemanticsMapper();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Reading Calendar", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Reading Calendar");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperReadingCalendar();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Chord Diagram", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Chord Diagram");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperChordDiagram();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Standup Tracker", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Standup Tracker");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperStandupTracker();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Rate Limiter", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Rate Limiter");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperRateLimiter();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Entity Linker", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Entity Linker");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperEntityLinker();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Reading Digest", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Reading Digest");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperReadingDigest();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Joy Plot", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Joy Plot");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperJoyPlot();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Incident Board", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Incident Board");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperIncidentBoard();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Health Check", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Health Check");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperHealthCheck();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Topic Cluster", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Topic Cluster");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperTopicCluster();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Reading Rhythm", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Reading Rhythm");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperReadingRhythm();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Polar Chart", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Polar Chart");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperPolarChart();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("On-Call Scheduler", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("On-Call Scheduler");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperOnCallScheduler();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Cache Warmer", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Cache Warmer");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperCacheWarmer();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Semantic Shift", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Semantic Shift");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperSemanticShift();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Reading Sprint Timer", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Reading Sprint Timer");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperReadingSprintTimer();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Swarm Plot", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Swarm Plot");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperSwarmPlot();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Task Dependency", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Task Dependency");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperTaskDependency();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Latency Profiler", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Latency Profiler");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperLatencyProfiler();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Concept Drift", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Concept Drift");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperConceptDrift();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Reading Zone Tracker", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Reading Zone Tracker");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperReadingZoneTracker();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Ridgeline Plot", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Ridgeline Plot");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperRidgelinePlot();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Meeting Notes", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Meeting Notes");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperMeetingNotes();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Schema Migrator", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Schema Migrator");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperSchemaMigrator();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Topic Sentiment", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Topic Sentiment");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperTopicSentiment();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Reading Flow State", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Reading Flow State");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperReadingFlowState();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Hexbin Plot", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Hexbin Plot");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperHexbinPlot();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Code Review Tracker", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Code Review Tracker");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperCodeReviewTracker();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("API Version Checker", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("API Version Checker");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperApiVersionChecker();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Citation Graph", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Citation Graph");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperCitationGraph();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Reading Session Sync", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Reading Session Sync");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperReadingSessionSync();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Streamgraph Plot", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Streamgraph Plot");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperStreamgraphPlot();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Sprint Retro Board", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Sprint Retro Board");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperSprintRetroBoard();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Endpoint Monitor", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Endpoint Monitor");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperEndpointMonitor();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Topic Clusterer", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Topic Clusterer");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperTopicClusterer2();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Reading Stamina Tracker", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Reading Stamina Tracker");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperReadingStaminaTracker();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Cleveland Dot Plot", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Cleveland Dot Plot");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperClevelandDotPlot();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Wiki Editor", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Wiki Editor");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperWikiEditor();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Cert Manager", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Cert Manager");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperCertManager();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Argument Evaluator", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Argument Evaluator");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperArgumentEvaluator();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Reading Bookmarks", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Reading Bookmarks");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperReadingBookmarks();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Marimekko Chart", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Marimekko Chart");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperMarimekkoChart2();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Asset Inventory", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Asset Inventory");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperAssetInventory();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Secret Scanner", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Secret Scanner");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperSecretScanner();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Citation Matcher", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Citation Matcher");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperCitationMatcher();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Reading Highlight", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Reading Highlight");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperReadingHighlight();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Waterfall Chart", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Waterfall Chart");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperWaterfallChart2();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("Team Calendar", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Team Calendar");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperTeamCalendar();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    toolsMenu->addAction("DNS Lookup Tool", this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("DNS Lookup Tool");
        dlg->setMinimumSize(600, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new PaperDnsLookupTool();
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });

    // Help menu
    QMenu* helpMenu = menuBar()->addMenu("&Help");

    QAction* aboutAction = helpMenu->addAction("&About");
    connect(aboutAction, &QAction::triggered, this, &MainWindow::onAbout);
}

void MainWindow::createToolBar() {
    QToolBar* toolBar = addToolBar("Main Toolbar");
    toolBar->setAllowedAreas(Qt::TopToolBarArea | Qt::BottomToolBarArea);
    toolBar->setStyleSheet(
        "QToolBar {"
        "  background-color: palette(window);"
        "  border: none;"
        "  spacing: 8px;"
        "  padding: 8px;"
        "  border-radius: 10px;"
        "  margin: 4px;"
        "}"
        "QToolButton {"
        "  background-color: transparent;"
        "  border-radius: 8px;"
        "  padding: 8px 16px;"
        "}"
        "QToolButton:hover {"
        "  background-color: rgba(99, 102, 241, 0.1);"
        "}"
    );

    QAction* searchAction = toolBar->addAction("🔍 Search");
    connect(searchAction, &QAction::triggered, this, [this]() {
        searchWidget_->setFocus();
        statusBar()->showMessage("Enter keyword in search box above", 3000);
    });

    toolBar->addSeparator();

    QAction* exportAction = toolBar->addAction("📥 Export");
    connect(exportAction, &QAction::triggered, this, &MainWindow::onExport);

    toolBar->addSeparator();

    QAction* settingsAction = toolBar->addAction("Settings");
    connect(settingsAction, &QAction::triggered, this, &MainWindow::onPreferences);

    toolBar->addSeparator();

    // Auth actions
    loginAction_ = toolBar->addAction("Login");
    connect(loginAction_, &QAction::triggered, this, &MainWindow::showLoginDialog);

    logoutAction_ = toolBar->addAction("Logout");
    logoutAction_->setVisible(false);
    connect(logoutAction_, &QAction::triggered, this, &MainWindow::handleLogout);

    profileAction_ = toolBar->addAction("Profile");
    profileAction_->setVisible(false);
    connect(profileAction_, &QAction::triggered, this, [this]() {
        if (!authManager_ || !authManager_->isAuthenticated()) return;
        DesktopUser user = authManager_->getCurrentUser();
        QString info = QString("Username: %1\nEmail: %2\nRole: %3")
            .arg(user.username, user.email, user.role);
        QMessageBox::information(this, "Profile", info);
    });

    toolBar->addSeparator();

    auto* favoritesAction = toolBar->addAction("Favorites");
    connect(favoritesAction, &QAction::triggered, this, [this]() {
        tabWidget_->setCurrentIndex(1);
    });

    // User label in status bar
    userLabel_ = new QLabel(this);
    statusBar()->addPermanentWidget(userLabel_);
}

void MainWindow::createThemeButton() {
    // Create floating theme toggle button
    themeButton_ = new QPushButton(this);
    themeButton_->setText("🌙");
    themeButton_->setToolTip("Toggle Dark/Light Theme");
    themeButton_->setObjectName("themeButton");
    themeButton_->setFixedSize(56, 56);
    themeButton_->setCursor(Qt::PointingHandCursor);

    // Position in top-right corner
    themeButton_->move(width() - 70, 80);

    // Modern button styling
    themeButton_->setStyleSheet(
        "QPushButton#themeButton {"
        "  background: qlineargradient(x1:0, y1:0, x2:1, y2:1, "
        "    stop:0 rgba(255, 255, 255, 0.95), "
        "    stop:1 rgba(243, 244, 246, 0.95));"
        "  border: 2px solid rgba(99, 102, 241, 0.3);"
        "  border-radius: 28px;"
        "  font-size: 24pt;"
        "  color: #6366f1;"
        "}"
        "QPushButton#themeButton:hover {"
        "  background: qlineargradient(x1:0, y1:0, x2:1, y2:1, "
        "    stop:0 #667eea, stop:1 #764ba2);"
        "  color: white;"
        "  transform: scale(1.1);"
        "}"
        "QPushButton#themeButton:pressed {"
        "  transform: scale(0.95);"
        "}"
    );

    connect(themeButton_, &QPushButton::clicked,
            this, &MainWindow::onToggleTheme);
}

void MainWindow::connectSignals() {
    connect(searchWidget_, &SearchWidget::searchRequested,
            this, &MainWindow::onSearch);

    // Search suggestions
    auto* searchEdit = searchWidget_->findChild<QLineEdit*>();
    if (searchEdit) {
        searchSuggest_ = new SearchSuggestWidget(searchEdit, this);
        connect(searchSuggest_, &SearchSuggestWidget::suggestionSelected,
                this, &MainWindow::onSearch);
    }

    // Recent history
    recentHistory_ = new RecentHistoryWidget(this);

    // Batch operations bar
    batchBar_ = new BatchOperationsBar(this);
    connect(batchBar_, &BatchOperationsBar::batchFavorite, this, [this](const QList<int>& ids, bool fav) {
        for (int id : ids) apiManager_->togglePaperFavorite(id, fav);
        ToastWidget::showSuccess(QString("%1 %2 papers").arg(fav ? "Favorited" : "Unfavorited").arg(ids.size()));
        batchBar_->clearSelection();
    });
    connect(batchBar_, &BatchOperationsBar::batchExport, this, [this](const QList<int>& ids) {
        Q_UNUSED(ids);
        onExport();
        batchBar_->clearSelection();
    });
    connect(batchBar_, &BatchOperationsBar::batchDelete, this, [this](const QList<int>& ids) {
        for (int id : ids) apiManager_->deletePaper(id);
        ToastWidget::showSuccess(QString("Deleted %1 papers").arg(ids.size()));
        batchBar_->clearSelection();
        if (!currentKeyword_.isEmpty()) {
            apiManager_->searchPapers(currentKeyword_, "", "", currentOffset_, currentLimit_);
        }
    });
    connect(batchBar_, &BatchOperationsBar::batchAddTags, this, [this](const QList<int>& ids, const QStringList& tags) {
        for (int id : ids) apiManager_->addPaperTags(id, tags);
        ToastWidget::showSuccess(QString("Added tags to %1 papers").arg(ids.size()));
        batchBar_->clearSelection();
    });

    // System tray
    trayManager_ = new SystemTrayManager(this);
    connect(trayManager_, &SystemTrayManager::showWindowRequested, this, [this]() {
        showNormal();
        activateWindow();
    });
    connect(trayManager_, &SystemTrayManager::searchRequested, this, [this](const QString& q) {
        showNormal();
        activateWindow();
        onSearch(q);
    });
    connect(trayManager_, &SystemTrayManager::quitRequested, this, []() {
        QApplication::quit();
    });

    // Notification center
    notificationCenter_ = new NotificationCenter(this);
    connect(notificationCenter_, &NotificationCenter::actionTriggered, this,
            [this](const QString& action) {
        if (action.startsWith("openPaper:")) {
            int paperId = action.mid(10).toInt();
            apiManager_->getPaperDetails(paperId);
        }
    });
    connect(notificationCenter_, &NotificationCenter::unreadCountChanged, this,
            [this](int count) {
        if (trayManager_) trayManager_->setUnreadCount(count);
    });

    // Drag & drop
    auto* dragDrop = new DragDropHandler(this);
    dragDrop->enableFor(this);
    connect(dragDrop, &DragDropHandler::textDropped, this, [this](const QString& text) {
        onSearch(text.trimmed());
        ToastWidget::showInfo("Searching dropped text...");
    });
    connect(dragDrop, &DragDropHandler::urlDropped, this, [this](const QUrl& url) {
        QString path = url.toString();
        if (path.contains("doi.org")) {
            ToastWidget::showInfo("DOI URL detected — use DOI Lookup to import");
        }
    });

    if (resultView_) {
        connect(resultView_, &PaperCardView::paperSelected,
                this, &MainWindow::onPaperSelected);
        connect(resultView_, &PaperCardView::pageChanged,
                this, &MainWindow::onPageChanged);
        resultView_->setContextMenuPolicy(Qt::CustomContextMenu);
        connect(resultView_, &PaperCardView::customContextMenuRequested, this,
                [this](const QPoint& pos) {
            auto papers = resultView_->getPapers();
            if (papers.isEmpty()) return;
            QModelIndex idx = resultView_->indexAt(pos);
            if (!idx.isValid()) return;
            int row = idx.row();
            if (row < 0 || row >= papers.size()) return;
            auto* menu = ContextMenuBuilder::buildPaperMenu(
                papers[row], apiManager_, nullptr, this);
            menu->exec(resultView_->viewport()->mapToGlobal(pos));
            menu->deleteLater();
        });
    }

    if (tableView_) {
        connect(tableView_, &ResultView::paperSelected,
                this, &MainWindow::onPaperSelected);
    }

    // Command palette (Ctrl+K)
    commandPalette_ = new CommandPalette(this);
    commandPalette_->addAction("Search Papers", "Ctrl+S", "Search", [this]() { searchWidget_->setFocus(); });
    commandPalette_->addAction("Advanced Search", "Ctrl+Shift+F", "Search", [this]() {
        auto* dlg = new AdvancedSearchDialog(this);
        connect(dlg, &AdvancedSearchDialog::searchRequested, this, [this](const AdvancedSearchDialog::SearchCriteria& c) {
            onSearch(c.query);
        });
        dlg->exec();
        dlg->deleteLater();
    });
    commandPalette_->addAction("New Paper", "", "Paper", [this]() { apiManager_->createPaper({}); });
    commandPalette_->addAction("Export", "Ctrl+E", "Paper", [this]() { onExport(); });
    commandPalette_->addAction("Toggle Theme", "Ctrl+T", "View", [this]() { onToggleTheme(); });
    commandPalette_->addAction("Theme Customizer", "", "View", [this]() {
        if (!themeCustomizer_) {
            themeCustomizer_ = new ThemeCustomizer(this);
            connect(themeCustomizer_, &ThemeCustomizer::themeChanged, this,
                    [this](const QMap<QString, QColor>&) { ToastWidget::showSuccess("Custom theme applied"); });
        }
        themeCustomizer_->exec();
    });
    commandPalette_->addAction("Advanced Export", "", "Paper", [this]() {
        if (!resultView_) return;
        auto papers = resultView_->getPapers();
        if (papers.isEmpty()) { ToastWidget::showWarning("No papers"); return; }
        auto* dlg = new PaperExportDialog(papers, this);
        dlg->exec();
        dlg->deleteLater();
    });
    commandPalette_->addAction("Paper Charts", "", "View", [this]() {
        ToastWidget::showInfo("Open Tools > Paper Charts");
    });
    commandPalette_->addAction("Quick Notes", "Ctrl+Shift+N", "Tools", [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Quick Notes");
        dlg->resize(400, 500);
        auto* layout = new QVBoxLayout(dlg);
        layout->addWidget(new QuickNoteWidget());
        dlg->exec();
        dlg->deleteLater();
    });
    commandPalette_->addAction("Paper Collections", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Paper Collections");
    });
    commandPalette_->addAction("Side-by-Side Diff", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Side-by-Side Diff");
    });
    commandPalette_->addAction("Reading Progress", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Reading Progress");
    });
    commandPalette_->addAction("Backup / Restore", "", "Tools", [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Backup & Restore");
        dlg->resize(600, 450);
        auto* layout = new QVBoxLayout(dlg);
        layout->addWidget(new BackupRestoreWidget());
        dlg->exec();
        dlg->deleteLater();
    });
    commandPalette_->addAction("Paper Recommendations", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Recommendations");
    });
    commandPalette_->addAction("Batch Import", "", "File", [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Batch Import");
        dlg->resize(800, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new BatchImportWidget();
        w->setApiManager(apiManager_);
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    commandPalette_->addAction("Tag Manager", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Tag Manager");
    });
    commandPalette_->addAction("Workspaces", "", "View", [this]() {
        ToastWidget::showInfo("Open Tools > Workspaces");
    });
    commandPalette_->addAction("Annotations", "", "Paper", [this]() {
        ToastWidget::showInfo("Open Tools > Annotations");
    });
    commandPalette_->addAction("Version History", "", "Paper", [this]() {
        ToastWidget::showInfo("Open Tools > Version History");
    });
    commandPalette_->addAction("Paper Rankings", "", "View", [this]() {
        ToastWidget::showInfo("Open Tools > Paper Rankings");
    });
    commandPalette_->addAction("AI Summarizer", "", "Tools", [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("AI Summarizer");
        dlg->resize(900, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* w = new AiSummarizerWidget();
        w->setApiManager(apiManager_);
        layout->addWidget(w);
        dlg->exec();
        dlg->deleteLater();
    });
    commandPalette_->addAction("Journal Browser", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Journal Browser");
    });
    commandPalette_->addAction("Paper Feed", "", "View", [this]() {
        ToastWidget::showInfo("Open Tools > Paper Feed");
    });
    commandPalette_->addAction("Hotkey Manager", "", "Settings", [this]() {
        ToastWidget::showInfo("Open Tools > Hotkey Manager");
    });
    commandPalette_->addAction("Citation Exporter", "", "Paper", [this]() {
        ToastWidget::showInfo("Open Tools > Citation Exporter");
    });
    commandPalette_->addAction("Reading Queue", "", "Paper", [this]() {
        ToastWidget::showInfo("Open Tools > Reading Queue");
    });
    commandPalette_->addAction("Collaboration", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Collaboration");
    });
    commandPalette_->addAction("Comparison Matrix", "", "Paper", [this]() {
        ToastWidget::showInfo("Open Tools > Comparison Matrix");
    });
    commandPalette_->addAction("PDF Viewer", "", "View", [this]() {
        ToastWidget::showInfo("Open Tools > PDF Viewer");
    });
    commandPalette_->addAction("Smart Search", "Ctrl+Shift+S", "Search", [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Smart Search");
        dlg->resize(700, 400);
        auto* layout = new QVBoxLayout(dlg);
        auto* ss = new SmartSearchWidget();
        connect(ss, &SmartSearchWidget::searchRequested, this, [this](const QString& q) {
            onSearch(q);
        });
        layout->addWidget(ss);
        dlg->exec();
        dlg->deleteLater();
    });
    commandPalette_->addAction("Paper Clustering", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Paper Clustering");
    });
    commandPalette_->addAction("Scheduled Tasks", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Scheduled Tasks");
    });
    commandPalette_->addAction("User Profile", "", "Settings", [this]() {
        ToastWidget::showInfo("Open Tools > User Profile");
    });
    commandPalette_->addAction("Mini Browser", "", "View", [this]() {
        ToastWidget::showInfo("Open Tools > Mini Browser");
    });
    commandPalette_->addAction("LaTeX Editor", "Ctrl+8", "Tabs", [this]() { tabWidget_->setCurrentIndex(7); });
    commandPalette_->addAction("AI Chat", "Ctrl+3", "Tabs", [this]() { tabWidget_->setCurrentIndex(2); });
    commandPalette_->addAction("Crawler", "Ctrl+4", "Tabs", [this]() { tabWidget_->setCurrentIndex(3); });
    commandPalette_->addAction("Recommendations", "Ctrl+5", "Tabs", [this]() { tabWidget_->setCurrentIndex(4); });
    commandPalette_->addAction("Admin", "Ctrl+6", "Tabs", [this]() { tabWidget_->setCurrentIndex(5); });
    commandPalette_->addAction("Statistics", "Ctrl+7", "Tabs", [this]() { tabWidget_->setCurrentIndex(6); });
    commandPalette_->addAction("DOI Lookup", "", "Tools", [this]() {
        auto* dlg = new DoiLookupDialog(this);
        connect(dlg, &DoiLookupDialog::paperFound, this, [this](const QJsonObject& data) {
            apiManager_->createPaper(data);
        });
        dlg->exec();
        dlg->deleteLater();
    });
    commandPalette_->addAction("Find Duplicates", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Find Duplicates");
    });
    commandPalette_->addAction("Paper Similarity", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Paper Similarity");
    });
    commandPalette_->addAction("Export Templates", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Export Templates");
    });
    commandPalette_->addAction("Keyboard Macros", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Keyboard Macros");
    });
    commandPalette_->addAction("Plugin Manager", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Plugin Manager");
    });
    commandPalette_->addAction("Widget Gallery", "", "View", [this]() {
        ToastWidget::showInfo("Open Tools > Widget Gallery");
    });
    commandPalette_->addAction("Paper Mind Map", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Paper Mind Map");
    });
    commandPalette_->addAction("Batch Download", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Batch Download");
    });
    commandPalette_->addAction("Reading Timer", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Reading Timer");
    });
    commandPalette_->addAction("Paper Grader", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Paper Grader");
    });
    commandPalette_->addAction("Data Visualization", "", "View", [this]() {
        ToastWidget::showInfo("Open Tools > Data Visualization");
    });
    commandPalette_->addAction("Paper Quiz", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Paper Quiz");
    });
    commandPalette_->addAction("Clipboard History", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Clipboard History");
    });
    commandPalette_->addAction("Translator", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Translator");
    });
    commandPalette_->addAction("Bibliography Builder", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Bibliography Builder");
    });
    commandPalette_->addAction("Language Detector", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Language Detector");
    });
    commandPalette_->addAction("Storyboard", "", "View", [this]() {
        ToastWidget::showInfo("Open Tools > Storyboard");
    });
    commandPalette_->addAction("Markdown Editor", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Markdown Editor");
    });
    commandPalette_->addAction("Query Builder", "", "Search", [this]() {
        ToastWidget::showInfo("Open Tools > Search Query Builder");
    });
    commandPalette_->addAction("Report Generator", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Report Generator");
    });
    commandPalette_->addAction("Author Network", "", "View", [this]() {
        ToastWidget::showInfo("Open Tools > Author Network");
    });
    commandPalette_->addAction("Reading Checklist", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Reading Checklist");
    });
    commandPalette_->addAction("Session Statistics", "", "View", [this]() {
        ToastWidget::showInfo("Open Tools > Session Statistics");
    });
    commandPalette_->addAction("Color Scheme", "", "Settings", [this]() {
        ToastWidget::showInfo("Open Tools > Color Scheme");
    });
    commandPalette_->addAction("Paper Merger", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Paper Merger");
    });
    commandPalette_->addAction("Abstract Summarizer", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Abstract Summarizer");
    });
    commandPalette_->addAction("Paper Dependencies", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Paper Dependencies");
    });
    commandPalette_->addAction("PDF Bookmarks", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > PDF Bookmarks");
    });
    commandPalette_->addAction("Paper Comparison", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Paper Comparison");
    });
    commandPalette_->addAction("Notification Rules", "", "Settings", [this]() {
        ToastWidget::showInfo("Open Tools > Notification Rules");
    });
    commandPalette_->addAction("Paper Timeline", "", "View", [this]() {
        ToastWidget::showInfo("Open Tools > Paper Timeline");
    });
    commandPalette_->addAction("Paper Comments", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Paper Comments");
    });
    commandPalette_->addAction("Paper Share", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Paper Share");
    });
    commandPalette_->addAction("Embedding View", "", "View", [this]() {
        ToastWidget::showInfo("Open Tools > Paper Embeddings");
    });
    commandPalette_->addAction("Reading Scheduler", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Reading Scheduler");
    });
    commandPalette_->addAction("Paper Templates", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Paper Templates");
    });
    commandPalette_->addAction("Annotations", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Paper Annotations");
    });
    commandPalette_->addAction("Citation Network", "", "View", [this]() {
        ToastWidget::showInfo("Open Tools > Citation Network");
    });
    commandPalette_->addAction("Bookmark Sync", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Bookmark Sync");
    });
    commandPalette_->addAction("Reading Progress", "", "View", [this]() {
        ToastWidget::showInfo("Open Tools > Reading Progress");
    });
    commandPalette_->addAction("Paper Ratings", "", "View", [this]() {
        ToastWidget::showInfo("Open Tools > Paper Ratings");
    });
    commandPalette_->addAction("Workflow Automator", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Workflow Automator");
    });
    commandPalette_->addAction("Graph Explorer", "", "View", [this]() {
        ToastWidget::showInfo("Open Tools > Citation Graph Explorer");
    });
    commandPalette_->addAction("Paper Insights", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Paper Insights");
    });
    commandPalette_->addAction("Reading Journal", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Reading Journal");
    });
    commandPalette_->addAction("Cross-References", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Cross-References");
    });
    commandPalette_->addAction("Peer Review", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Peer Review");
    });
    commandPalette_->addAction("Literature Matrix", "", "View", [this]() {
        ToastWidget::showInfo("Open Tools > Literature Matrix");
    });
    commandPalette_->addAction("Concept Map", "", "View", [this]() {
        ToastWidget::showInfo("Open Tools > Paper Concepts");
    });
    commandPalette_->addAction("Reading Streak", "", "View", [this]() {
        ToastWidget::showInfo("Open Tools > Reading Streak");
    });
    commandPalette_->addAction("Batch Export", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Batch Export");
    });
    commandPalette_->addAction("Summarizer Chain", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Summarizer Chain");
    });
    commandPalette_->addAction("Research Trends", "", "View", [this]() {
        ToastWidget::showInfo("Open Tools > Research Trends");
    });
    commandPalette_->addAction("Duplicate Detector", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Duplicate Detector");
    });
    commandPalette_->addAction("Reading Goals", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Reading Goals");
    });
    commandPalette_->addAction("Knowledge Base", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Knowledge Base");
    });
    commandPalette_->addAction("Reference Extractor", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Reference Extractor");
    });
    commandPalette_->addAction("Search History", "", "View", [this]() {
        ToastWidget::showInfo("Open Tools > Search History Analysis");
    });
    commandPalette_->addAction("Citation Style", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Citation Style Editor");
    });
    commandPalette_->addAction("Session Log", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Session Log");
    });
    commandPalette_->addAction("Figure Extractor", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Figure Extractor");
    });
    commandPalette_->addAction("Topic Modeler", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Topic Modeler");
    });
    commandPalette_->addAction("Reading Speed Analyzer", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Reading Speed Analyzer");
    });
    commandPalette_->addAction("Citation Counter", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Citation Counter");
    });
    commandPalette_->addAction("Keyword Extractor", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Keyword Extractor");
    });
    commandPalette_->addAction("Reading Plan", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Reading Plan");
    });
    commandPalette_->addAction("Collaboration Board", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Collaboration Board");
    });
    commandPalette_->addAction("Research Notes", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Research Notes");
    });
    commandPalette_->addAction("Feedback Collector", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Feedback Collector");
    });
    commandPalette_->addAction("Literature Timeline", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Literature Timeline");
    });
    commandPalette_->addAction("Tag Cloud", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Tag Cloud");
    });
    commandPalette_->addAction("Reading Heatmap", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Reading Heatmap");
    });
    commandPalette_->addAction("Citation Style Generator", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Citation Style Generator");
    });
    commandPalette_->addAction("Version Diff", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Version Diff");
    });
    commandPalette_->addAction("Research Calendar", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Research Calendar");
    });
    commandPalette_->addAction("Sentiment Analyzer", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Sentiment Analyzer");
    });
    commandPalette_->addAction("Citation Network", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Citation Network");
    });
    commandPalette_->addAction("Co-Reading Tracker", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Co-Reading Tracker");
    });
    commandPalette_->addAction("Paper QA Chat", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Paper QA Chat");
    });
    commandPalette_->addAction("Trend Heatmap", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Trend Heatmap");
    });
    commandPalette_->addAction("Auto Tagger", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Auto Tagger");
    });
    commandPalette_->addAction("Reading Score", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Reading Score");
    });
    commandPalette_->addAction("Math Formula Renderer", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Math Formula Renderer");
    });
    commandPalette_->addAction("Reading Groups", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Reading Groups");
    });
    commandPalette_->addAction("Alert Monitor", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Alert Monitor");
    });
    commandPalette_->addAction("Geography Map", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Geography Map");
    });
    commandPalette_->addAction("Citation Radar Chart", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Citation Radar Chart");
    });
    commandPalette_->addAction("Research Timeline", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Research Timeline");
    });
    commandPalette_->addAction("Argument Parser", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Argument Parser");
    });
    commandPalette_->addAction("Code Snippet Manager", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Code Snippet Manager");
    });
    commandPalette_->addAction("Resource Allocator", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Resource Allocator");
    });
    commandPalette_->addAction("Influence Graph", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Influence Graph");
    });
    commandPalette_->addAction("Methodology Extractor", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Methodology Extractor");
    });
    commandPalette_->addAction("Data Transform", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Data Transform");
    });
    commandPalette_->addAction("Experiment Tracker", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Experiment Tracker");
    });
    commandPalette_->addAction("Reading Notes Organizer", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Reading Notes Organizer");
    });
    commandPalette_->addAction("Collaboration Heatmap", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Collaboration Heatmap");
    });
    commandPalette_->addAction("Bias Detector", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Bias Detector");
    });
    commandPalette_->addAction("Paper Validation", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Paper Validation");
    });
    commandPalette_->addAction("Grant Tracker", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Grant Tracker");
    });
    commandPalette_->addAction("Reading Challenges", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Reading Challenges");
    });
    commandPalette_->addAction("Citation Tree Map", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Citation Tree Map");
    });
    commandPalette_->addAction("Reproducibility Checker", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Reproducibility Checker");
    });
    commandPalette_->addAction("Pomodoro Timer", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Pomodoro Timer");
    });
    commandPalette_->addAction("Dataset Manager", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Dataset Manager");
    });
    commandPalette_->addAction("Reading Analytics", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Reading Analytics");
    });
    commandPalette_->addAction("Workflow Sankey", "", "Visualization", [this]() {
        ToastWidget::showInfo("Open Tools > Workflow Sankey");
    });
    commandPalette_->addAction("Entity Extractor", "", "Analysis", [this]() {
        ToastWidget::showInfo("Open Tools > Entity Extractor");
    });
    commandPalette_->addAction("Side-by-Side Compare", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Side-by-Side Compare");
    });
    commandPalette_->addAction("Conference Planner", "", "Workspace", [this]() {
        ToastWidget::showInfo("Open Tools > Conference Planner");
    });
    commandPalette_->addAction("Peer Review Tracker", "", "Citation", [this]() {
        ToastWidget::showInfo("Open Tools > Peer Review Tracker");
    });
    commandPalette_->addAction("Survey Builder", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Survey Builder");
    });
    commandPalette_->addAction("Citation Context Analyzer", "", "Analysis", [this]() {
        ToastWidget::showInfo("Open Tools > Citation Context Analyzer");
    });
    commandPalette_->addAction("Reading Path Optimizer", "", "Reading", [this]() {
        ToastWidget::showInfo("Open Tools > Reading Path Optimizer");
    });
    commandPalette_->addAction("Knowledge Graph Explorer", "", "Visualization", [this]() {
        ToastWidget::showInfo("Open Tools > Knowledge Graph Explorer");
    });
    commandPalette_->addAction("Author Disambiguator", "", "Citation", [this]() {
        ToastWidget::showInfo("Open Tools > Author Disambiguator");
    });
    commandPalette_->addAction("Citation Timing Analyzer", "", "Analysis", [this]() {
        ToastWidget::showInfo("Open Tools > Citation Timing Analyzer");
    });
    commandPalette_->addAction("Reading List Sorter", "", "Reading", [this]() {
        ToastWidget::showInfo("Open Tools > Reading List Sorter");
    });
    commandPalette_->addAction("Trend Predictor", "", "Visualization", [this]() {
        ToastWidget::showInfo("Open Tools > Trend Predictor");
    });
    commandPalette_->addAction("Collaboration Matcher", "", "Workspace", [this]() {
        ToastWidget::showInfo("Open Tools > Collaboration Matcher");
    });
    commandPalette_->addAction("Formula Search Engine", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Formula Search Engine");
    });
    commandPalette_->addAction("Recommender Feedback", "", "Analysis", [this]() {
        ToastWidget::showInfo("Open Tools > Recommender Feedback");
    });
    commandPalette_->addAction("Reading Speed Tracker", "", "Reading", [this]() {
        ToastWidget::showInfo("Open Tools > Reading Speed Tracker");
    });
    commandPalette_->addAction("Citation Network D3", "", "Visualization", [this]() {
        ToastWidget::showInfo("Open Tools > Citation Network D3");
    });
    commandPalette_->addAction("Project Timeline", "", "Workspace", [this]() {
        ToastWidget::showInfo("Open Tools > Project Timeline");
    });
    commandPalette_->addAction("LaTeX Symbol Finder", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > LaTeX Symbol Finder");
    });
    commandPalette_->addAction("Argument Strength Analyzer", "", "Analysis", [this]() {
        ToastWidget::showInfo("Open Tools > Argument Strength Analyzer");
    });
    commandPalette_->addAction("Reading Mood Tracker", "", "Reading", [this]() {
        ToastWidget::showInfo("Open Tools > Reading Mood Tracker");
    });
    commandPalette_->addAction("Citation Impact Cloud", "", "Visualization", [this]() {
        ToastWidget::showInfo("Open Tools > Citation Impact Cloud");
    });
    commandPalette_->addAction("Budget Planner", "", "Workspace", [this]() {
        ToastWidget::showInfo("Open Tools > Budget Planner");
    });
    commandPalette_->addAction("Batch Renamer", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Batch Renamer");
    });
    commandPalette_->addAction("Semantic Similarity Matrix", "", "Analysis", [this]() {
        ToastWidget::showInfo("Open Tools > Semantic Similarity Matrix");
    });
    commandPalette_->addAction("Reading Retention Tracker", "", "Reading", [this]() {
        ToastWidget::showInfo("Open Tools > Reading Retention Tracker");
    });
    commandPalette_->addAction("Citation Flow Diagram", "", "Visualization", [this]() {
        ToastWidget::showInfo("Open Tools > Citation Flow Diagram");
    });
    commandPalette_->addAction("Ethics Checker", "", "Workspace", [this]() {
        ToastWidget::showInfo("Open Tools > Ethics Checker");
    });
    commandPalette_->addAction("Regex Search Tool", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Regex Search Tool");
    });
    commandPalette_->addAction("Contradiction Detector", "", "Analysis", [this]() {
        ToastWidget::showInfo("Open Tools > Contradiction Detector");
    });
    commandPalette_->addAction("Annotation Sync", "", "Reading", [this]() {
        ToastWidget::showInfo("Open Tools > Annotation Sync");
    });
    commandPalette_->addAction("Research Heatmap", "", "Visualization", [this]() {
        ToastWidget::showInfo("Open Tools > Research Heatmap");
    });
    commandPalette_->addAction("Collaboration Score", "", "Workspace", [this]() {
        ToastWidget::showInfo("Open Tools > Collaboration Score");
    });
    commandPalette_->addAction("Markdown Exporter", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Markdown Exporter");
    });
    commandPalette_->addAction("Claim Strength Analyzer", "", "Analysis", [this]() {
        ToastWidget::showInfo("Open Tools > Claim Strength Analyzer");
    });
    commandPalette_->addAction("Reading Session Timer", "", "Reading", [this]() {
        ToastWidget::showInfo("Open Tools > Reading Session Timer");
    });
    commandPalette_->addAction("Keyword Evolution", "", "Visualization", [this]() {
        ToastWidget::showInfo("Open Tools > Keyword Evolution");
    });
    commandPalette_->addAction("Review Assignment", "", "Workspace", [this]() {
        ToastWidget::showInfo("Open Tools > Review Assignment");
    });
    commandPalette_->addAction("Table Extractor", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Table Extractor");
    });
    commandPalette_->addAction("Methodology Comparator", "", "Analysis", [this]() {
        ToastWidget::showInfo("Open Tools > Methodology Comparator");
    });
    commandPalette_->addAction("Reading Speed Test", "", "Reading", [this]() {
        ToastWidget::showInfo("Open Tools > Reading Speed Test");
    });
    commandPalette_->addAction("Citation Sankey", "", "Visualization", [this]() {
        ToastWidget::showInfo("Open Tools > Citation Sankey");
    });
    commandPalette_->addAction("Project Milestone", "", "Workspace", [this]() {
        ToastWidget::showInfo("Open Tools > Project Milestone");
    });
    commandPalette_->addAction("Figure Captioner", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Figure Captioner");
    });
    commandPalette_->addAction("Evidence Strength", "", "Analysis", [this]() {
        ToastWidget::showInfo("Open Tools > Evidence Strength");
    });
    commandPalette_->addAction("Vocabulary Tracker", "", "Reading", [this]() {
        ToastWidget::showInfo("Open Tools > Vocabulary Tracker");
    });
    commandPalette_->addAction("Trend Sparkline", "", "Visualization", [this]() {
        ToastWidget::showInfo("Open Tools > Trend Sparkline");
    });
    commandPalette_->addAction("Team Availability", "", "Workspace", [this]() {
        ToastWidget::showInfo("Open Tools > Team Availability");
    });
    commandPalette_->addAction("PDF Annotator", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > PDF Annotator");
    });
    commandPalette_->addAction("Novelty Scorer", "", "Analysis", [this]() {
        ToastWidget::showInfo("Open Tools > Novelty Scorer");
    });
    commandPalette_->addAction("Reading Goals", "", "Reading", [this]() {
        ToastWidget::showInfo("Open Tools > Reading Goals");
    });
    commandPalette_->addAction("Impact Treemap", "", "Visualization", [this]() {
        ToastWidget::showInfo("Open Tools > Impact Treemap");
    });
    commandPalette_->addAction("Budget Forecast", "", "Workspace", [this]() {
        ToastWidget::showInfo("Open Tools > Budget Forecast");
    });
    commandPalette_->addAction("Code Snippet Extractor", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Code Snippet Extractor");
    });
    commandPalette_->addAction("Claim Verifier", "", "Analysis", [this]() {
        ToastWidget::showInfo("Open Tools > Claim Verifier");
    });
    commandPalette_->addAction("Reading Milestone", "", "Reading", [this]() {
        ToastWidget::showInfo("Open Tools > Reading Milestone");
    });
    commandPalette_->addAction("Donut Chart", "", "Visualization", [this]() {
        ToastWidget::showInfo("Open Tools > Donut Chart");
    });
    commandPalette_->addAction("Invoice Tracker", "", "Workspace", [this]() {
        ToastWidget::showInfo("Open Tools > Invoice Tracker");
    });
    commandPalette_->addAction("API Tester", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > API Tester");
    });
    commandPalette_->addAction("Evidence Chain", "", "Analysis", [this]() {
        ToastWidget::showInfo("Open Tools > Evidence Chain");
    });
    commandPalette_->addAction("Reading Challenge", "", "Reading", [this]() {
        ToastWidget::showInfo("Open Tools > Reading Challenge");
    });
    commandPalette_->addAction("Bubble Chart", "", "Visualization", [this]() {
        ToastWidget::showInfo("Open Tools > Bubble Chart");
    });
    commandPalette_->addAction("Expense Logger", "", "Workspace", [this]() {
        ToastWidget::showInfo("Open Tools > Expense Logger");
    });
    commandPalette_->addAction("Web Scraper", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Web Scraper");
    });
    commandPalette_->addAction("Hypothesis Tracker", "", "Analysis", [this]() {
        ToastWidget::showInfo("Open Tools > Hypothesis Tracker");
    });
    commandPalette_->addAction("Reading Puzzle", "", "Reading", [this]() {
        ToastWidget::showInfo("Open Tools > Reading Puzzle");
    });
    commandPalette_->addAction("Radar Chart", "", "Visualization", [this]() {
        ToastWidget::showInfo("Open Tools > Radar Chart");
    });
    commandPalette_->addAction("Contract Manager", "", "Workspace", [this]() {
        ToastWidget::showInfo("Open Tools > Contract Manager");
    });
    commandPalette_->addAction("Cron Scheduler", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Cron Scheduler");
    });
    commandPalette_->addAction("Argument Mapper", "", "Analysis", [this]() {
        ToastWidget::showInfo("Open Tools > Argument Mapper");
    });
    commandPalette_->addAction("Reading Badge", "", "Reading", [this]() {
        ToastWidget::showInfo("Open Tools > Reading Badge");
    });
    commandPalette_->addAction("Heatmap Grid", "", "Visualization", [this]() {
        ToastWidget::showInfo("Open Tools > Heatmap Grid");
    });
    commandPalette_->addAction("Proposal Tracker", "", "Workspace", [this]() {
        ToastWidget::showInfo("Open Tools > Proposal Tracker");
    });
    commandPalette_->addAction("Log Viewer", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Log Viewer");
    });
    commandPalette_->addAction("Fallacy Detector", "", "Analysis", [this]() {
        ToastWidget::showInfo("Open Tools > Fallacy Detector");
    });
    commandPalette_->addAction("Reading Streak Board", "", "Reading", [this]() {
        ToastWidget::showInfo("Open Tools > Reading Streak Board");
    });
    commandPalette_->addAction("Scatter Plot", "", "Visualization", [this]() {
        ToastWidget::showInfo("Open Tools > Scatter Plot");
    });
    commandPalette_->addAction("Vendor Rating", "", "Workspace", [this]() {
        ToastWidget::showInfo("Open Tools > Vendor Rating");
    });
    commandPalette_->addAction("Config Editor", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Config Editor");
    });
    commandPalette_->addAction("Bias Checker", "", "Analysis", [this]() {
        ToastWidget::showInfo("Open Tools > Bias Checker");
    });
    commandPalette_->addAction("Reading Gamification", "", "Reading", [this]() {
        ToastWidget::showInfo("Open Tools > Reading Gamification");
    });
    commandPalette_->addAction("Contour Plot", "", "Visualization", [this]() {
        ToastWidget::showInfo("Open Tools > Contour Plot");
    });
    commandPalette_->addAction("License Tracker", "", "Workspace", [this]() {
        ToastWidget::showInfo("Open Tools > License Tracker");
    });
    commandPalette_->addAction("Environment Switcher", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Environment Switcher");
    });
    commandPalette_->addAction("Sentiment Tracker", "", "Analysis", [this]() {
        ToastWidget::showInfo("Open Tools > Sentiment Tracker");
    });
    commandPalette_->addAction("Reading Achievement", "", "Reading", [this]() {
        ToastWidget::showInfo("Open Tools > Reading Achievement");
    });
    commandPalette_->addAction("Waterfall Chart", "", "Visualization", [this]() {
        ToastWidget::showInfo("Open Tools > Waterfall Chart");
    });
    commandPalette_->addAction("Compliance Monitor", "", "Workspace", [this]() {
        ToastWidget::showInfo("Open Tools > Compliance Monitor");
    });
    commandPalette_->addAction("Log Analyzer", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Log Analyzer");
    });
    commandPalette_->addAction("Topic Clusterer", "", "Analysis", [this]() {
        ToastWidget::showInfo("Open Tools > Topic Clusterer");
    });
    commandPalette_->addAction("Reading Dashboard", "", "Reading", [this]() {
        ToastWidget::showInfo("Open Tools > Reading Dashboard");
    });
    commandPalette_->addAction("Funnel Chart", "", "Visualization", [this]() {
        ToastWidget::showInfo("Open Tools > Funnel Chart");
    });
    commandPalette_->addAction("Audit Logger", "", "Workspace", [this]() {
        ToastWidget::showInfo("Open Tools > Audit Logger");
    });
    commandPalette_->addAction("Health Monitor", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Health Monitor");
    });
    commandPalette_->addAction("Argument Strength", "", "Analysis", [this]() {
        ToastWidget::showInfo("Open Tools > Argument Strength");
    });
    commandPalette_->addAction("Reading Scorecard", "", "Reading", [this]() {
        ToastWidget::showInfo("Open Tools > Reading Scorecard");
    });
    commandPalette_->addAction("Gauge Chart", "", "Visualization", [this]() {
        ToastWidget::showInfo("Open Tools > Gauge Chart");
    });
    commandPalette_->addAction("Policy Manager", "", "Workspace", [this]() {
        ToastWidget::showInfo("Open Tools > Policy Manager");
    });
    commandPalette_->addAction("Cache Inspector", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Cache Inspector");
    });
    commandPalette_->addAction("Concept Extractor", "", "Analysis", [this]() {
        ToastWidget::showInfo("Open Tools > Concept Extractor");
    });
    commandPalette_->addAction("Reading Heatmap Grid", "", "Reading", [this]() {
        ToastWidget::showInfo("Open Tools > Reading Heatmap Grid");
    });
    commandPalette_->addAction("Sankey Diagram", "", "Visualization", [this]() {
        ToastWidget::showInfo("Open Tools > Sankey Diagram");
    });
    commandPalette_->addAction("Risk Assessor", "", "Workspace", [this]() {
        ToastWidget::showInfo("Open Tools > Risk Assessor");
    });
    commandPalette_->addAction("Feature Flag Manager", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Feature Flag Manager");
    });
    commandPalette_->addAction("Entity Recognizer", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Entity Recognizer");
    });
    commandPalette_->addAction("Reading Progress Grid", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Reading Progress Grid");
    });
    commandPalette_->addAction("Treemap Chart", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Treemap Chart");
    });
    commandPalette_->addAction("Cost Estimator", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Cost Estimator");
    });
    commandPalette_->addAction("Plugin Installer", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Plugin Installer");
    });
    commandPalette_->addAction("Argument Visualizer", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Argument Visualizer");
    });
    commandPalette_->addAction("Score Predictor", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Score Predictor");
    });
    commandPalette_->addAction("Word Cloud", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Word Cloud");
    });
    commandPalette_->addAction("Vendor Comparison", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Vendor Comparison");
    });
    commandPalette_->addAction("Deployment Tracker", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Deployment Tracker");
    });
    commandPalette_->addAction("Citation Analyzer", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Citation Analyzer");
    });
    commandPalette_->addAction("Reading Cluster", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Reading Cluster");
    });
    commandPalette_->addAction("Heatmap Widget", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Heatmap Widget");
    });
    commandPalette_->addAction("Sprint Board", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Sprint Board");
    });
    commandPalette_->addAction("Keyboard Shortcut", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Keyboard Shortcut");
    });
    commandPalette_->addAction("Claim Tracker", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Claim Tracker");
    });
    commandPalette_->addAction("Reading Calendar", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Reading Calendar");
    });
    commandPalette_->addAction("Network Graph 3D", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Network Graph 3D");
    });
    commandPalette_->addAction("Time Tracker", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Time Tracker");
    });
    commandPalette_->addAction("Theme Builder", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Theme Builder");
    });
    commandPalette_->addAction("Sentiment Explorer", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Sentiment Explorer");
    });
    commandPalette_->addAction("Reading Competency", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Reading Competency");
    });
    commandPalette_->addAction("Parallel Coordinates", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Parallel Coordinates");
    });
    commandPalette_->addAction("Resource Scheduler", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Resource Scheduler");
    });
    commandPalette_->addAction("Script Runner", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Script Runner");
    });
    commandPalette_->addAction("Topic Evolution", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Topic Evolution");
    });
    commandPalette_->addAction("Reading Flow", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Reading Flow");
    });
    commandPalette_->addAction("Violin Plot", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Violin Plot");
    });
    commandPalette_->addAction("Meeting Scheduler", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Meeting Scheduler");
    });
    commandPalette_->addAction("Context Menu Editor", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Context Menu Editor");
    });
    commandPalette_->addAction("Argument Miner", "", "Analysis", [this]() {
        ToastWidget::showInfo("Open Tools > Argument Miner");
    });
    commandPalette_->addAction("Reading Workshop", "", "Reading", [this]() {
        ToastWidget::showInfo("Open Tools > Reading Workshop");
    });
    commandPalette_->addAction("Radar Spinner", "", "Visualization", [this]() {
        ToastWidget::showInfo("Open Tools > Radar Spinner");
    });
    commandPalette_->addAction("Inventory Tracker", "", "Workspace", [this]() {
        ToastWidget::showInfo("Open Tools > Inventory Tracker");
    });
    commandPalette_->addAction("Notification Filter", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Notification Filter");
    });
    commandPalette_->addAction("Evidence Evaluator", "", "Analysis", [this]() {
        ToastWidget::showInfo("Open Tools > Evidence Evaluator");
    });
    commandPalette_->addAction("Annotation Studio", "", "Reading", [this]() {
        ToastWidget::showInfo("Open Tools > Annotation Studio");
    });
    commandPalette_->addAction("Flow Diagram", "", "Visualization", [this]() {
        ToastWidget::showInfo("Open Tools > Flow Diagram");
    });
    commandPalette_->addAction("Funding Monitor", "", "Workspace", [this]() {
        ToastWidget::showInfo("Open Tools > Funding Monitor");
    });
    commandPalette_->addAction("Update Checker", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Update Checker");
    });
    commandPalette_->addAction("Bias Scanner", "", "Analysis", [this]() {
        ToastWidget::showInfo("Open Tools > Bias Scanner");
    });
    commandPalette_->addAction("Citation Logger", "", "Reading", [this]() {
        ToastWidget::showInfo("Open Tools > Citation Logger");
    });
    commandPalette_->addAction("Bubble Matrix", "", "Visualization", [this]() {
        ToastWidget::showInfo("Open Tools > Bubble Matrix");
    });
    commandPalette_->addAction("Task Automator", "", "Workspace", [this]() {
        ToastWidget::showInfo("Open Tools > Task Automator");
    });
    commandPalette_->addAction("Log Inspector", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Log Inspector");
    });
    commandPalette_->addAction("Concept Gap", "", "Analysis", [this]() {
        ToastWidget::showInfo("Open Tools > Concept Gap");
    });
    commandPalette_->addAction("Skill Assessor", "", "Reading", [this]() {
        ToastWidget::showInfo("Open Tools > Skill Assessor");
    });
    commandPalette_->addAction("Gantt Chart", "", "Visualization", [this]() {
        ToastWidget::showInfo("Open Tools > Gantt Chart");
    });
    commandPalette_->addAction("Paper Recommender", "", "Workspace", [this]() {
        ToastWidget::showInfo("Open Tools > Paper Recommender");
    });
    commandPalette_->addAction("Snippet Vault", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Snippet Vault");
    });
    commandPalette_->addAction("Novelty Score", "", "Analysis", [this]() {
        ToastWidget::showInfo("Open Tools > Novelty Score");
    });
    commandPalette_->addAction("Study Planner", "", "Reading", [this]() {
        ToastWidget::showInfo("Open Tools > Study Planner");
    });
    commandPalette_->addAction("Dot Plot", "", "Visualization", [this]() {
        ToastWidget::showInfo("Open Tools > Dot Plot");
    });
    commandPalette_->addAction("Lab Notebook", "", "Workspace", [this]() {
        ToastWidget::showInfo("Open Tools > Lab Notebook");
    });
    commandPalette_->addAction("Version Tree", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Version Tree");
    });
    commandPalette_->addAction("Claim Validator", "", "Analysis", [this]() {
        ToastWidget::showInfo("Open Tools > Claim Validator");
    });
    commandPalette_->addAction("Reading Journal", "", "Reading", [this]() {
        ToastWidget::showInfo("Open Tools > Reading Journal");
    });
    commandPalette_->addAction("Stacked Chart", "", "Visualization", [this]() {
        ToastWidget::showInfo("Open Tools > Stacked Chart");
    });
    commandPalette_->addAction("Team Workspace", "", "Workspace", [this]() {
        ToastWidget::showInfo("Open Tools > Team Workspace");
    });
    commandPalette_->addAction("Markup Editor", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Markup Editor");
    });
    commandPalette_->addAction("Cohort Analyzer", "", "Analysis", [this]() {
        ToastWidget::showInfo("Open Tools > Cohort Analyzer");
    });
    commandPalette_->addAction("Flashcard Deck", "", "Reading", [this]() {
        ToastWidget::showInfo("Open Tools > Flashcard Deck");
    });
    commandPalette_->addAction("Ring Chart", "", "Visualization", [this]() {
        ToastWidget::showInfo("Open Tools > Ring Chart");
    });
    commandPalette_->addAction("Milestone Tracker", "", "Workspace", [this]() {
        ToastWidget::showInfo("Open Tools > Milestone Tracker");
    });
    commandPalette_->addAction("Plugin Manager", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Plugin Manager");
    });
    commandPalette_->addAction("Contradiction Finder", "", "Analysis", [this]() {
        ToastWidget::showInfo("Open Tools > Contradiction Finder");
    });
    commandPalette_->addAction("Spaced Repetition", "", "Reading", [this]() {
        ToastWidget::showInfo("Open Tools > Spaced Repetition");
    });
    commandPalette_->addAction("Area Chart", "", "Visualization", [this]() {
        ToastWidget::showInfo("Open Tools > Area Chart");
    });
    commandPalette_->addAction("Risk Matrix", "", "Workspace", [this]() {
        ToastWidget::showInfo("Open Tools > Risk Matrix");
    });
    commandPalette_->addAction("Macro Recorder", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Macro Recorder");
    });
    commandPalette_->addAction("Hypothesis Tester", "", "Analysis", [this]() {
        ToastWidget::showInfo("Open Tools > Hypothesis Tester");
    });
    commandPalette_->addAction("Vocabulary Builder", "", "Reading", [this]() {
        ToastWidget::showInfo("Open Tools > Vocabulary Builder");
    });
    commandPalette_->addAction("Candlestick Chart", "", "Visualization", [this]() {
        ToastWidget::showInfo("Open Tools > Candlestick Chart");
    });
    commandPalette_->addAction("Budget Tracker", "", "Workspace", [this]() {
        ToastWidget::showInfo("Open Tools > Budget Tracker");
    });
    commandPalette_->addAction("Color Picker", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Color Picker");
    });
    commandPalette_->addAction("Correlation Engine", "", "Analysis", [this]() {
        ToastWidget::showInfo("Open Tools > Correlation Engine");
    });
    commandPalette_->addAction("Quiz Generator", "", "Reading", [this]() {
        ToastWidget::showInfo("Open Tools > Quiz Generator");
    });
    commandPalette_->addAction("Pyramid Chart", "", "Visualization", [this]() {
        ToastWidget::showInfo("Open Tools > Pyramid Chart");
    });
    commandPalette_->addAction("Team Scoreboard", "", "Workspace", [this]() {
        ToastWidget::showInfo("Open Tools > Team Scoreboard");
    });
    commandPalette_->addAction("Font Browser", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Font Browser");
    });
    commandPalette_->addAction("Factor Analyzer", "", "Analysis", [this]() {
        ToastWidget::showInfo("Open Tools > Factor Analyzer");
    });
    commandPalette_->addAction("Note Organizer", "", "Reading", [this]() {
        ToastWidget::showInfo("Open Tools > Note Organizer");
    });
    commandPalette_->addAction("Bullet Chart", "", "Visualization", [this]() {
        ToastWidget::showInfo("Open Tools > Bullet Chart");
    });
    commandPalette_->addAction("Asset Manager", "", "Workspace", [this]() {
        ToastWidget::showInfo("Open Tools > Asset Manager");
    });
    commandPalette_->addAction("Regex Tester", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Regex Tester");
    });
    commandPalette_->addAction("Outlier Detector", "", "Analysis", [this]() {
        ToastWidget::showInfo("Open Tools > Outlier Detector");
    });
    commandPalette_->addAction("Paper Timeline", "", "Reading", [this]() {
        ToastWidget::showInfo("Open Tools > Paper Timeline");
    });
    commandPalette_->addAction("Lollipop Chart", "", "Visualization", [this]() {
        ToastWidget::showInfo("Open Tools > Lollipop Chart");
    });
    commandPalette_->addAction("Conference Tracker", "", "Workspace", [this]() {
        ToastWidget::showInfo("Open Tools > Conference Tracker");
    });
    commandPalette_->addAction("Diff Viewer", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Diff Viewer");
    });
    commandPalette_->addAction("Cluster Validator", "", "Analysis", [this]() {
        ToastWidget::showInfo("Open Tools > Cluster Validator");
    });
    commandPalette_->addAction("Reference Graph", "", "Reading", [this]() {
        ToastWidget::showInfo("Open Tools > Reference Graph");
    });
    commandPalette_->addAction("Histogram Widget", "", "Visualization", [this]() {
        ToastWidget::showInfo("Open Tools > Histogram Widget");
    });
    commandPalette_->addAction("Experiment Logger", "", "Workspace", [this]() {
        ToastWidget::showInfo("Open Tools > Experiment Logger");
    });
    commandPalette_->addAction("Archive Manager", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Archive Manager");
    });
    commandPalette_->addAction("Topic Tracker", "", "Analysis", [this]() {
        ToastWidget::showInfo("Open Tools > Topic Tracker");
    });
    commandPalette_->addAction("Reading Thermometer", "", "Reading", [this]() {
        ToastWidget::showInfo("Open Tools > Reading Thermometer");
    });
    commandPalette_->addAction("Step Chart", "", "Visualization", [this]() {
        ToastWidget::showInfo("Open Tools > Step Chart");
    });
    commandPalette_->addAction("Project Board", "", "Workspace", [this]() {
        ToastWidget::showInfo("Open Tools > Project Board");
    });
    commandPalette_->addAction("Clipboard Manager", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Clipboard Manager");
    });
    commandPalette_->addAction("Anomaly Detector", "", "Analysis", [this]() {
        ToastWidget::showInfo("Open Tools > Anomaly Detector");
    });
    commandPalette_->addAction("Reading Radar", "", "Reading", [this]() {
        ToastWidget::showInfo("Open Tools > Reading Radar");
    });
    commandPalette_->addAction("Treemap Widget", "", "Visualization", [this]() {
        ToastWidget::showInfo("Open Tools > Treemap Widget");
    });
    commandPalette_->addAction("Survey Manager", "", "Workspace", [this]() {
        ToastWidget::showInfo("Open Tools > Survey Manager");
    });
    commandPalette_->addAction("Bookmark Organizer", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Bookmark Organizer");
    });
    commandPalette_->addAction("Pattern Miner", "", "Analysis", [this]() {
        ToastWidget::showInfo("Open Tools > Pattern Miner");
    });
    commandPalette_->addAction("Reading Pulse", "", "Reading", [this]() {
        ToastWidget::showInfo("Open Tools > Reading Pulse");
    });
    commandPalette_->addAction("Radial Chart", "", "Visualization", [this]() {
        ToastWidget::showInfo("Open Tools > Radial Chart");
    });
    commandPalette_->addAction("Collaboration Hub", "", "Workspace", [this]() {
        ToastWidget::showInfo("Open Tools > Collaboration Hub");
    });
    commandPalette_->addAction("Code Vault", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Code Vault");
    });
    commandPalette_->addAction("Bias Monitor", "", "Analysis", [this]() {
        ToastWidget::showInfo("Open Tools > Bias Monitor");
    });
    commandPalette_->addAction("Reading Vitals", "", "Reading", [this]() {
        ToastWidget::showInfo("Open Tools > Reading Vitals");
    });
    commandPalette_->addAction("Waffle Chart", "", "Visualization", [this]() {
        ToastWidget::showInfo("Open Tools > Waffle Chart");
    });
    commandPalette_->addAction("Team Sync", "", "Workspace", [this]() {
        ToastWidget::showInfo("Open Tools > Team Sync");
    });
    commandPalette_->addAction("Formula Editor", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Formula Editor");
    });
    commandPalette_->addAction("Causal Finder", "", "Analysis", [this]() {
        ToastWidget::showInfo("Open Tools > Causal Finder");
    });
    commandPalette_->addAction("Reading Compass", "", "Reading", [this]() {
        ToastWidget::showInfo("Open Tools > Reading Compass");
    });
    commandPalette_->addAction("Marimekko Chart", "", "Visualization", [this]() {
        ToastWidget::showInfo("Open Tools > Marimekko Chart");
    });
    commandPalette_->addAction("Resource Pool", "", "Workspace", [this]() {
        ToastWidget::showInfo("Open Tools > Resource Pool");
    });
    commandPalette_->addAction("Unit Converter", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Unit Converter");
    });
    commandPalette_->addAction("Relevance Scorer", "", "Analysis", [this]() {
        ToastWidget::showInfo("Open Tools > Relevance Scorer");
    });
    commandPalette_->addAction("Reading Sprint", "", "Reading", [this]() {
        ToastWidget::showInfo("Open Tools > Reading Sprint");
    });
    commandPalette_->addAction("Parallel Plot", "", "Visualization", [this]() {
        ToastWidget::showInfo("Open Tools > Parallel Plot");
    });
    commandPalette_->addAction("Version Tracker", "", "Workspace", [this]() {
        ToastWidget::showInfo("Open Tools > Version Tracker");
    });
    commandPalette_->addAction("Timer Widget", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Timer Widget");
    });
    commandPalette_->addAction("Semantic Mapper", "", "Analysis", [this]() {
        ToastWidget::showInfo("Open Tools > Semantic Mapper");
    });
    commandPalette_->addAction("Reading Waves", "", "Reading", [this]() {
        ToastWidget::showInfo("Open Tools > Reading Waves");
    });
    commandPalette_->addAction("Dendrogram View", "", "Visualization", [this]() {
        ToastWidget::showInfo("Open Tools > Dendrogram View");
    });
    commandPalette_->addAction("Workflow Board", "", "Workspace", [this]() {
        ToastWidget::showInfo("Open Tools > Workflow Board");
    });
    commandPalette_->addAction("Checksum Tool", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Checksum Tool");
    });
    commandPalette_->addAction("Embedding Visualizer", "", "Analysis", [this]() {
        ToastWidget::showInfo("Open Tools > Embedding Visualizer");
    });
    commandPalette_->addAction("Reading Gradient", "", "Reading", [this]() {
        ToastWidget::showInfo("Open Tools > Reading Gradient");
    });
    commandPalette_->addAction("Heatmap Matrix", "", "Visualization", [this]() {
        ToastWidget::showInfo("Open Tools > Heatmap Matrix");
    });
    commandPalette_->addAction("Pipeline View", "", "Workspace", [this]() {
        ToastWidget::showInfo("Open Tools > Pipeline View");
    });
    commandPalette_->addAction("Diff Tool", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Diff Tool");
    });
    commandPalette_->addAction("Knowledge Extractor", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Knowledge Extractor");
    });
    commandPalette_->addAction("Reading Elevation", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Reading Elevation");
    });
    commandPalette_->addAction("Box Plot", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Box Plot");
    });
    commandPalette_->addAction("Timeline Gantt", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Timeline Gantt");
    });
    commandPalette_->addAction("File Watcher", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > File Watcher");
    });
    commandPalette_->addAction("Novelty Score", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Novelty Score");
    });
    commandPalette_->addAction("Study Planner", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Study Planner");
    });
    commandPalette_->addAction("Dot Plot", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Dot Plot");
    });
    commandPalette_->addAction("Lab Notebook", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Lab Notebook");
    });
    commandPalette_->addAction("Version Tree", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Version Tree");
    });
    commandPalette_->addAction("Contrast Analyzer", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Contrast Analyzer");
    });
    commandPalette_->addAction("Reading Goal Tree", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Reading Goal Tree");
    });
    commandPalette_->addAction("Lollipop Chart", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Lollipop Chart");
    });
    commandPalette_->addAction("Experiment Tracker", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Experiment Tracker");
    });
    commandPalette_->addAction("Log Rotator", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Log Rotator");
    });
    commandPalette_->addAction("Claim Verifier", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Claim Verifier");
    });
    commandPalette_->addAction("Reading Heatmap", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Reading Heatmap");
    });
    commandPalette_->addAction("Range Chart", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Range Chart");
    });
    commandPalette_->addAction("Protocol Manager", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Protocol Manager");
    });
    commandPalette_->addAction("Config Differ", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Config Differ");
    });
    commandPalette_->addAction("Evidence Weigher", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Evidence Weigher");
    });
    commandPalette_->addAction("Reading Depth", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Reading Depth");
    });
    commandPalette_->addAction("Bullet Chart", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Bullet Chart");
    });
    commandPalette_->addAction("Hypothesis Board", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Hypothesis Board");
    });
    commandPalette_->addAction("Patch Manager", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Patch Manager");
    });
    commandPalette_->addAction("Bias Detector", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Bias Detector");
    });
    commandPalette_->addAction("Reading Scorecard", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Reading Scorecard");
    });
    commandPalette_->addAction("Funnel Chart", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Funnel Chart");
    });
    commandPalette_->addAction("Task Board", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Task Board");
    });
    commandPalette_->addAction("Token Counter", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Token Counter");
    });
    commandPalette_->addAction("Reproducibility Checker", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Reproducibility Checker");
    });
    commandPalette_->addAction("Reading Pace", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Reading Pace");
    });
    commandPalette_->addAction("Candlestick Chart", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Candlestick Chart");
    });
    commandPalette_->addAction("Milestone Tracker", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Milestone Tracker");
    });
    commandPalette_->addAction("Service Monitor", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Service Monitor");
    });
    commandPalette_->addAction("Argument Strength", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Argument Strength");
    });
    commandPalette_->addAction("Reading Journal", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Reading Journal");
    });
    commandPalette_->addAction("Waterfall Chart", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Waterfall Chart");
    });
    commandPalette_->addAction("Release Planner", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Release Planner");
    });
    commandPalette_->addAction("Deprecation Tracker", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Deprecation Tracker");
    });
    commandPalette_->addAction("Citation Verifier", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Citation Verifier");
    });
    commandPalette_->addAction("Reading Backlog", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Reading Backlog");
    });
    commandPalette_->addAction("Radar Chart", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Radar Chart");
    });
    commandPalette_->addAction("Sprint Board", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Sprint Board");
    });
    commandPalette_->addAction("Key Store", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Key Store");
    });
    commandPalette_->addAction("Impact Predictor", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Impact Predictor");
    });
    commandPalette_->addAction("Reading Velocity", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Reading Velocity");
    });
    commandPalette_->addAction("Sankey Chart", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Sankey Chart");
    });
    commandPalette_->addAction("Retrospective", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Retrospective");
    });
    commandPalette_->addAction("Secret Vault", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Secret Vault");
    });
    commandPalette_->addAction("Trend Detector", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Trend Detector");
    });
    commandPalette_->addAction("Reading Binge", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Reading Binge");
    });
    commandPalette_->addAction("Tornado Chart", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Tornado Chart");
    });
    commandPalette_->addAction("Capacity Planner", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Capacity Planner");
    });
    commandPalette_->addAction("Feature Flag", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Feature Flag");
    });
    commandPalette_->addAction("Citation Network", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Citation Network");
    });
    commandPalette_->addAction("Reading Streak", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Reading Streak");
    });
    commandPalette_->addAction("Violin Chart", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Violin Chart");
    });
    commandPalette_->addAction("Dependency Graph", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Dependency Graph");
    });
    commandPalette_->addAction("Audit Log", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Audit Log");
    });
    commandPalette_->addAction("Semantics Mapper", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Semantics Mapper");
    });
    commandPalette_->addAction("Reading Calendar", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Reading Calendar");
    });
    commandPalette_->addAction("Chord Diagram", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Chord Diagram");
    });
    commandPalette_->addAction("Standup Tracker", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Standup Tracker");
    });
    commandPalette_->addAction("Rate Limiter", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Rate Limiter");
    });
    commandPalette_->addAction("Entity Linker", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Entity Linker");
    });
    commandPalette_->addAction("Reading Digest", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Reading Digest");
    });
    commandPalette_->addAction("Joy Plot", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Joy Plot");
    });
    commandPalette_->addAction("Incident Board", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Incident Board");
    });
    commandPalette_->addAction("Health Check", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Health Check");
    });
    commandPalette_->addAction("Topic Cluster", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Topic Cluster");
    });
    commandPalette_->addAction("Reading Rhythm", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Reading Rhythm");
    });
    commandPalette_->addAction("Polar Chart", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Polar Chart");
    });
    commandPalette_->addAction("On-Call Scheduler", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > On-Call Scheduler");
    });
    commandPalette_->addAction("Cache Warmer", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Cache Warmer");
    });
    commandPalette_->addAction("Semantic Shift", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Semantic Shift");
    });
    commandPalette_->addAction("Reading Sprint Timer", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Reading Sprint Timer");
    });
    commandPalette_->addAction("Swarm Plot", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Swarm Plot");
    });
    commandPalette_->addAction("Task Dependency", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Task Dependency");
    });
    commandPalette_->addAction("Latency Profiler", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Latency Profiler");
    });
    commandPalette_->addAction("Concept Drift", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Concept Drift");
    });
    commandPalette_->addAction("Reading Zone Tracker", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Reading Zone Tracker");
    });
    commandPalette_->addAction("Ridgeline Plot", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Ridgeline Plot");
    });
    commandPalette_->addAction("Meeting Notes", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Meeting Notes");
    });
    commandPalette_->addAction("Schema Migrator", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Schema Migrator");
    });
    commandPalette_->addAction("Topic Sentiment", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Topic Sentiment");
    });
    commandPalette_->addAction("Reading Flow State", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Reading Flow State");
    });
    commandPalette_->addAction("Hexbin Plot", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Hexbin Plot");
    });
    commandPalette_->addAction("Code Review Tracker", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Code Review Tracker");
    });
    commandPalette_->addAction("API Version Checker", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > API Version Checker");
    });
    commandPalette_->addAction("Citation Graph", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Citation Graph");
    });
    commandPalette_->addAction("Reading Session Sync", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Reading Session Sync");
    });
    commandPalette_->addAction("Streamgraph Plot", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Streamgraph Plot");
    });
    commandPalette_->addAction("Sprint Retro Board", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Sprint Retro Board");
    });
    commandPalette_->addAction("Endpoint Monitor", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Endpoint Monitor");
    });
    commandPalette_->addAction("Topic Clusterer", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Topic Clusterer");
    });
    commandPalette_->addAction("Reading Stamina Tracker", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Reading Stamina Tracker");
    });
    commandPalette_->addAction("Cleveland Dot Plot", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Cleveland Dot Plot");
    });
    commandPalette_->addAction("Wiki Editor", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Wiki Editor");
    });
    commandPalette_->addAction("Cert Manager", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Cert Manager");
    });
    commandPalette_->addAction("Argument Evaluator", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Argument Evaluator");
    });
    commandPalette_->addAction("Reading Bookmarks", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Reading Bookmarks");
    });
    commandPalette_->addAction("Marimekko Chart", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Marimekko Chart");
    });
    commandPalette_->addAction("Asset Inventory", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Asset Inventory");
    });
    commandPalette_->addAction("Secret Scanner", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Secret Scanner");
    });
    commandPalette_->addAction("Citation Matcher", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Citation Matcher");
    });
    commandPalette_->addAction("Reading Highlight", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Reading Highlight");
    });
    commandPalette_->addAction("Waterfall Chart", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Waterfall Chart");
    });
    commandPalette_->addAction("Team Calendar", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Team Calendar");
    });
    commandPalette_->addAction("DNS Lookup Tool", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > DNS Lookup Tool");
    });
    commandPalette_->addAction("Reading Lists", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Reading Lists");
    });
    commandPalette_->addAction("Offline Cache", "", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Offline Cache");
    });
    commandPalette_->addAction("Recent History", "Ctrl+H", "Tools", [this]() {
        ToastWidget::showInfo("Open Tools > Recent History");
    });
    commandPalette_->addAction("Keyboard Shortcuts", "", "Help", [this]() {
        auto* dlg = new ShortcutConfigDialog(this);
        dlg->exec();
        dlg->deleteLater();
    });
    commandPalette_->addAction("About", "", "Help", [this]() { onAbout(); });
    commandPalette_->addAction("Quit", "Ctrl+Q", "File", []() { QApplication::quit(); });

    auto* cmdPaletteShortcut = new QShortcut(QKeySequence("Ctrl+K"), this);
    connect(cmdPaletteShortcut, &QShortcut::activated, this, [this]() {
        commandPalette_->showPalette();
    });

    // Session manager
    sessionManager_ = new SessionManager(this);
    sessionManager_->saveRecentSearch("");

    // Update checker
    updateChecker_ = new UpdateChecker(this);
    updateChecker_->setCurrentVersion("1.0.0");
    connect(updateChecker_, &UpdateChecker::updateAvailable, this,
            [this](const QString& version, const QString& url, const QString& notes) {
        if (notificationCenter_) {
            notificationCenter_->addNotification(Notification::Info, "Update Available",
                QString("Version %1 available").arg(version), "");
        }
        statusBar()->showMessage(QString("Update %1 available").arg(version), 10000);
    });
    updateChecker_->checkForUpdates();

    // Connect API signals
    connect(apiManager_, &ApiManager::searchSuccess,
            this, &MainWindow::onSearchSuccess);
    connect(apiManager_, &ApiManager::searchFailed,
            this, &MainWindow::onSearchFailed);
    connect(apiManager_, &ApiManager::healthCheckSuccess,
            this, &MainWindow::onHealthCheckSuccess);
    connect(apiManager_, &ApiManager::paperDetailsSuccess,
            this, &MainWindow::onPaperDetailsSuccess);
    connect(apiManager_, &ApiManager::networkError,
            this, &MainWindow::onNetworkError);
    connect(apiManager_, &ApiManager::recentPapersSuccess,
            this, [this](const QList<Paper>& papers) {
        auto* grid = findChild<QWidget*>("recentPapersGrid");
        if (!grid) return;
        auto* gridLayout = grid->findChild<QHBoxLayout*>("recentGridLayout");
        if (!gridLayout) return;

        // Clear old cards
        QLayoutItem* item;
        while ((item = gridLayout->takeAt(0)) != nullptr) {
            delete item->widget();
            delete item;
        }

        for (const auto& paper : papers) {
            auto* card = new QWidget();
            card->setStyleSheet(
                "QWidget { background: palette(base); border: 1px solid palette(mid); "
                "border-radius: 8px; }"
            );
            card->setCursor(Qt::PointingHandCursor);
            card->setMaximumWidth(280);
            card->setMinimumHeight(100);

            auto* layout = new QVBoxLayout(card);
            layout->setContentsMargins(12, 10, 12, 10);
            layout->setSpacing(4);

            auto* titleLabel = new QLabel(paper.title.left(80));
            titleLabel->setWordWrap(true);
            titleLabel->setStyleSheet("font-weight: bold; font-size: 12px; color: palette(text);");
            layout->addWidget(titleLabel);

            auto* meta = new QLabel(
                (paper.journalShort.isEmpty() ? paper.journalFull : paper.journalShort)
                + " | " + paper.year
            );
            meta->setStyleSheet("font-size: 10px; color: palette(mid);");
            layout->addWidget(meta);

            int pid = paper.id;
            connect(card, &QWidget::mousePressEvent, this, [this, pid](QMouseEvent*) {
                onPaperSelected(pid);
            });

            gridLayout->addWidget(card);
        }
    });

    // Handle API failure signals
    connect(apiManager_, &ApiManager::healthCheckFailed, this, [this](const QString& error) {
        auto* healthLabel = findChild<QLabel*>("healthIndicator");
        if (!healthLabel) {
            healthLabel = new QLabel(this);
            healthLabel->setObjectName("healthIndicator");
            statusBar()->addPermanentWidget(healthLabel);
        }
        healthLabel->setStyleSheet(
            "padding: 2px 8px; border-radius: 4px; font-size: 11px; font-weight: bold; "
            "color: #dc2626; background: #fee2e2;"
        );
        healthLabel->setText("Offline");
        setWindowTitle("PaperCrawler - Offline");
    });

    connect(apiManager_, &ApiManager::paperDetailsFailed, this, [this](const QString& error) {
        statusBar()->showMessage("Failed to load paper details: " + error.left(50), 5000);
    });

    connect(apiManager_, &ApiManager::statsFailed, this, [this](const QString& error) {
        auto* statsContent = findChild<QLabel*>("statsContent");
        if (statsContent) {
            statsContent->setText("<p style='color: #dc2626;'>Failed to load statistics. Server may be offline.</p>");
        }
    });

    connect(apiManager_, &ApiManager::apiError, this, [this](const QString& error) {
        statusBar()->showMessage("API error: " + error.left(50), 5000);
    });

    // Filter panel
    if (filterPanel_) {
        connect(filterPanel_, &FilterPanel::filterChanged,
                this, [this](const QString& level, const QString& year, const QString& type) {
            Q_UNUSED(type);
            if (!currentKeyword_.isEmpty()) {
                currentOffset_ = 0;
                apiManager_->searchPapers(currentKeyword_, year, level, 0, currentLimit_);
            }
        });
    }

    // Tab switch data loading
    connect(tabWidget_, &QTabWidget::currentChanged, this, [this](int index) {
        switch (index) {
            case 1: // Favorites
                refreshFavoritesTab();
                break;
            case 2: // Crawler - auto-refresh running tasks
                apiManager_->getCrawlerDashboard();
                apiManager_->getCrawlerTasks();
                apiManager_->getCrawlerTemplates();
                apiManager_->getCrawlerSchedules();
                break;
            case 4: // Statistics
                apiManager_->getStats("overview");
                break;
            case 5: // Recommendations
                apiManager_->getPaperRecommendations();
                break;
            case 6: // Admin
                apiManager_->getAdminDashboard();
                apiManager_->getAdminUsers();
                apiManager_->getAdminModules();
                apiManager_->getSystemMonitor();
                break;
            case 7: // LaTeX Editor
                apiManager_->listLatexDocuments();
                apiManager_->listLatexTemplates();
                break;
        }
    });

    // Crawler auto-refresh timer (poll every 15s when on crawler tab)
    auto* crawlerRefreshTimer = new QTimer(this);
    connect(crawlerRefreshTimer, &QTimer::timeout, this, [this]() {
        if (tabWidget_->currentIndex() == 2) {
            apiManager_->getCrawlerDashboard();
            apiManager_->getCrawlerTasks();
        }
    });
    crawlerRefreshTimer->start(15000);

    // === Recommendations: generic JSON responses ===
    connect(apiManager_, &ApiManager::jsonResponse, this,
        [this](const QString& endpoint, const QJsonObject& data) {
            if (endpoint.contains("recommendations")) {
                populateRecommendations(data);
            } else if (endpoint.contains("trending")) {
                QJsonArray papers = data["papers"].toArray(data["data"].toArray());
                QJsonObject wrapped;
                wrapped["papers"] = papers;
                wrapped["total"] = papers.size();
                wrapped["source"] = "trending";
                populateRecommendations(wrapped);
            } else if (endpoint.contains("similar")) {
                QJsonArray papers = data["papers"].toArray(data["similar"].toArray());
                QJsonObject wrapped;
                wrapped["papers"] = papers;
                wrapped["total"] = papers.size();
                wrapped["source"] = "similar";
                populateRecommendations(wrapped);
            } else if (endpoint.contains("admin/dashboard")) {
                populateAdminDashboard(data);
            } else if (endpoint.contains("admin/users")) {
                populateAdminUsers(data);
            } else if (endpoint.contains("admin/modules") || endpoint.contains("modules")) {
                populateAdminModules(data);
            } else if (endpoint.contains("monitor")) {
                populateAdminMonitor(data);
            } else if (endpoint.contains("performance")) {
                populatePerformanceMetrics(data);
            } else if (endpoint.contains("login")) {
                showLoginHistory(data);
            } else if (endpoint.contains("recommendation") && endpoint.contains("feedback")) {
                statusBar()->showMessage("Feedback submitted. Thank you!", 3000);
            } else if (endpoint.contains("recommendation") && endpoint.contains("explanation")) {
                showRecommendationExplanationDialog(data);
            } else if (endpoint.contains("ai/summarize")) {
                auto* aiChat = findChild<QTextEdit*>("aiChatDisplay");
                if (aiChat) aiChat->append("<b>AI Summary:</b> " + data["summary"].toString(data["text"].toString("Summary completed.")));
            } else if (endpoint.contains("ai/compare")) {
                auto* aiChat = findChild<QTextEdit*>("aiChatDisplay");
                if (aiChat) aiChat->append("<b>AI Compare:</b> " + data["comparison"].toString(data["text"].toString("Comparison completed.")));
            } else if (endpoint.contains("ai/keywords")) {
                auto* aiChat = findChild<QTextEdit*>("aiChatDisplay");
                QString kws = data["keywords"].toString(data["text"].toString());
                if (kws.isEmpty() && data.contains("keywords")) {
                    QJsonArray kwArr = data["keywords"].toArray();
                    QStringList kwList;
                    for (const auto& kw : kwArr) kwList << kw.toString();
                    kws = kwList.join(", ");
                }
                if (aiChat) aiChat->append("<b>AI Keywords:</b> " + (kws.isEmpty() ? "Keywords extraction completed." : kws));
            } else if (endpoint.contains("ai/contributions")) {
                auto* aiChat = findChild<QTextEdit*>("aiChatDisplay");
                if (aiChat) aiChat->append("<b>AI Contributions:</b> " + data["contributions"].toString(data["text"].toString("Contributions analysis completed.")));
            } else if (endpoint.contains("ai/copilot/review")) {
                auto* aiChat = findChild<QTextEdit*>("aiChatDisplay");
                if (aiChat) aiChat->append("<b>Copilot Review:</b> " + data["review"].toString(data["text"].toString("Copilot review completed.")));
            } else if (endpoint.contains("ai/literature")) {
                auto* aiChat = findChild<QTextEdit*>("aiChatDisplay");
                QString review = data["review"].toString(data["literature_review"].toString(data["text"].toString()));
                if (aiChat) aiChat->append("<b>Literature Review:</b> " + (review.isEmpty() ? "Generated successfully." : review));
            } else if (endpoint.contains("ai/research")) {
                auto* aiChat = findChild<QTextEdit*>("aiChatDisplay");
                QString plan = data["plan"].toString(data["research_plan"].toString(data["text"].toString()));
                if (aiChat) aiChat->append("<b>Research Plan:</b> " + (plan.isEmpty() ? "Generated successfully." : plan));
            } else if (endpoint.contains("ai/status")) {
                showAiStatusDialog(data);
            } else if (endpoint.contains("ai/copilot/recommendations")) {
                auto* aiChat = findChild<QTextEdit*>("aiChatDisplay");
                if (aiChat) aiChat->append("<b>Copilot Recommendations:</b> " + data["recommendations"].toString(data["text"].toString("Recommendations loaded.")));
            } else if (endpoint.contains("ai/copilot/stats")) {
                auto* aiChat = findChild<QTextEdit*>("aiChatDisplay");
                if (aiChat) aiChat->append("<b>Copilot Stats:</b> Reviews: " +
                    QString::number(data["total_reviews"].toInt(0)) +
                    " | Plans: " + QString::number(data["total_plans"].toInt(0)));
            } else if (endpoint.contains("crawler/schedules")) {
                populateCrawlerSchedules(data);
            } else if (endpoint.contains("crawler/workers")) {
                showCrawlerWorkersDialog(data);
            } else if (endpoint.contains("crawler/statistics")) {
                showCrawlerStatisticsDialog(data);
            } else if (endpoint.contains("crawler/templates")) {
                populateCrawlerTemplates(data);
            } else if (endpoint.contains("export/formats")) {
                statusBar()->showMessage("Export formats loaded", 2000);
            } else if (endpoint.contains("search/saved")) {
                populateSavedSearches(data);
            } else if (endpoint.contains("search/trending")) {
                populateTrendingSearches(data);
            } else if (endpoint.contains("search/history")) {
                showSearchHistoryDialog(data);
            } else if (endpoint.contains("papers") && endpoint.contains("all")) {
                showAllPapersDialog(data);
            } else if (endpoint.contains("stats/system")) {
                showStatsDialog("System Statistics", data);
            } else if (endpoint.contains("stats/resource")) {
                showStatsDialog("Resource Statistics", data);
            } else if (endpoint.contains("stats/performance")) {
                showStatsDialog("Performance Statistics", data);
            } else if (endpoint.contains("latex/documents") && endpoint.contains("create")) {
                // New document created
                int docId = data["id"].toInt(0);
                if (docId > 0 && latexEditor_) {
                    statusBar()->showMessage(QString("Document created (ID: %1)").arg(docId), 5000);
                    apiManager_->listLatexDocuments();
                }
            } else if (endpoint.contains("latex/documents/update")) {
                statusBar()->showMessage("Document saved", 2000);
            } else if (endpoint.contains("latex/documents/delete")) {
                statusBar()->showMessage("Document deleted", 3000);
                apiManager_->listLatexDocuments();
            } else if (endpoint.contains("latex/compile")) {
                if (latexEditor_) {
                    bool ok = data["success"].toBool();
                    QString pdf = data["pdfPath"].toString(data["pdf_path"].toString());
                    QString err = data["error"].toString(data["errorMessage"].toString());
                    auto* preview = latexEditor_->findChild<LatexPreviewWidget*>();
                    if (preview) preview->showCompileResult(ok, pdf, err);
                }
                statusBar()->showMessage(data["success"].toBool() ? "Compilation successful" : "Compilation failed", 5000);
            } else if (endpoint.contains("latex/autosave")) {
                // Silent auto-save
            } else if (endpoint.contains("latex/templates")) {
                showLatexTemplatesDialog(data);
            } else if (endpoint.contains("latex/document") && !endpoint.contains("create") && !endpoint.contains("update") && !endpoint.contains("delete")) {
                // Single document loaded
                if (latexEditor_) {
                    latexEditor_->loadContent(data["content"].toString());
                }
            }
        });

    connect(apiManager_, &ApiManager::jsonArrayResponse, this,
        [this](const QString& endpoint, const QJsonArray& data) {
            if (endpoint.contains("recommendations")) {
                QJsonObject wrapped;
                wrapped["papers"] = data;
                wrapped["total"] = data.size();
                wrapped["source"] = "recommendations";
                populateRecommendations(wrapped);
            } else if (endpoint.contains("trending")) {
                QJsonObject wrapped;
                wrapped["papers"] = data;
                wrapped["total"] = data.size();
                wrapped["source"] = "trending";
                populateRecommendations(wrapped);
            } else if (endpoint.contains("admin/users")) {
                QJsonObject wrapped;
                wrapped["users"] = data;
                populateAdminUsers(wrapped);
            } else if (endpoint.contains("admin/modules") || endpoint.contains("modules")) {
                QJsonObject wrapped;
                wrapped["modules"] = data;
                populateAdminModules(wrapped);
            } else if (endpoint.contains("crawler/schedules")) {
                QJsonObject wrapped;
                wrapped["schedules"] = data;
                populateCrawlerSchedules(wrapped);
            } else if (endpoint.contains("crawler/templates")) {
                QJsonObject wrapped;
                wrapped["templates"] = data;
                populateCrawlerTemplates(wrapped);
            } else if (endpoint.contains("search/saved")) {
                QJsonObject wrapped;
                wrapped["searches"] = data;
                populateSavedSearches(wrapped);
            } else if (endpoint.contains("search/trending")) {
                QJsonObject wrapped;
                wrapped["trending"] = data;
                populateTrendingSearches(wrapped);
            } else if (endpoint.contains("latex/documents")) {
                // Document list (array) — populate combo
                if (latexEditor_) {
                    auto* combo = latexEditor_->findChild<QComboBox*>();
                    if (combo) {
                        combo->clear();
                        combo->addItem("New Document", 0);
                        for (const auto& val : data) {
                            auto d = val.toObject();
                            combo->addItem(d["title"].toString(d["name"].toString()), d["id"].toInt());
                        }
                    }
                }
            } else if (endpoint.contains("latex/templates")) {
                showLatexTemplatesDialog(data);
            }
        });

    connect(apiManager_, &ApiManager::genericError, this,
        [this](const QString& context, const QString& error) {
            statusBar()->showMessage(
                QString("%1 failed: %2").arg(context, error.left(80)), 5000);
        });
}

void MainWindow::loadSettings() {
    QSettings settings("PaperCrawler", "Desktop");

    restoreGeometry(settings.value("geometry").toByteArray());
    restoreState(settings.value("windowState").toByteArray());

    // Apply saved theme
    int themeIndex = settings.value("display/theme", 0).toInt();
    if (themeIndex == 2 && themeManager_) {
        // Dark
        if (themeManager_->currentTheme() != ThemeManager::ThemeMode::Dark) {
            themeManager_->toggleTheme();
            themeButton_->setText("\xe2\x98\x80\xef\xb8\x8f");
        }
    } else if (themeIndex == 1 && themeManager_) {
        // Light
        if (themeManager_->currentTheme() == ThemeManager::ThemeMode::Dark) {
            themeManager_->toggleTheme();
            themeButton_->setText("\xf0\x9f\x8c\x99");
        }
    }

    // Apply saved page size
    int savedPageSize = settings.value("display/pageSize", 20).toInt();
    currentLimit_ = savedPageSize;

    // Apply saved server URL
    QString savedUrl = settings.value("server/url", "http://localhost:8080").toString();
    if (apiManager_) {
        apiManager_->setBaseUrl(savedUrl);
        if (authManager_) {
            authManager_->setBaseUrl(savedUrl);
        }
    }
}

void MainWindow::saveSettings() {
    QSettings settings("PaperCrawler", "Desktop");

    settings.setValue("geometry", saveGeometry());
    settings.setValue("windowState", saveState());

    // Save theme preference
    if (themeManager_) {
        int themeIndex = themeManager_->currentTheme() == ThemeManager::ThemeMode::Dark ? 2 : 1;
        settings.setValue("display/theme", themeIndex);
    }
    settings.setValue("display/pageSize", currentLimit_);
}

void MainWindow::onSearch(const QString& keyword) {
    if (keyword.isEmpty()) {
        QMessageBox::warning(this, "Search", "Please enter a search keyword.");
        return;
    }

    // Save search state for pagination
    currentKeyword_ = keyword;
    currentOffset_ = 0;
    currentLimit_ = 20;

    statusBar()->showMessage("Searching: " + keyword + "...");
    resultView_->setVisible(true);
    resultView_->clear();
    resultView_->setHighlightKeyword(keyword);
    filterPanel_->setVisible(true);

    // Hide hero section when searching
    if (heroWidget_) heroWidget_->setVisible(false);
    if (featureCards_) featureCards_->setVisible(false);

    // Show loading indicator in status bar
    auto* progressBar = findChild<QProgressBar*>("searchProgress");
    if (!progressBar) {
        progressBar = new QProgressBar(this);
        progressBar->setObjectName("searchProgress");
        progressBar->setRange(0, 0);  // Indeterminate
        progressBar->setMaximumWidth(200);
        progressBar->setMaximumHeight(16);
        progressBar->setTextVisible(false);
        progressBar->setStyleSheet(
            "QProgressBar { border: 1px solid palette(mid); border-radius: 4px; background: palette(base); }"
            "QProgressBar::chunk { background: #4f46e5; border-radius: 3px; }"
        );
        statusBar()->addPermanentWidget(progressBar);
    }
    progressBar->setVisible(true);

    // Clear cache for this keyword if starting fresh search
    // (Optional: keep cache for faster access if same keyword searched again)
    // paperCache_->clear(keyword);

    // Check cache first
    QList<Paper> cachedPapers;
    int cachedTotal = 0;

    if (paperCache_->get(keyword, 0, 20, cachedPapers, cachedTotal)) {
        qDebug() << "Fresh search found in cache! Displaying cached results.";
        resultView_->setPapers(cachedPapers, cachedTotal, 1);  // Page 1
        statusBar()->showMessage(QString("搜索完成（来自缓存）！找到 %1 篇相关论文").arg(cachedTotal), 5000);
        return;
    }

    // TODO: Add local database search later
    // For now, directly search backend
    apiManager_->searchPapers(keyword, "", "", currentOffset_, currentLimit_);
}

void MainWindow::onPageChanged(int offset, int limit) {
    qDebug() << "=== MainWindow::onPageChanged ===";
    qDebug() << "Offset:" << offset << "Limit:" << limit;
    qDebug() << "Current keyword:" << currentKeyword_;

    if (currentKeyword_.isEmpty()) {
        qWarning() << "No current keyword, ignoring page change";
        return;
    }

    currentOffset_ = offset;
    currentLimit_ = limit;

    int pageNum = (offset / limit) + 1;

    // Try to get from cache first
    QList<Paper> cachedPapers;
    int cachedTotal = 0;

    if (paperCache_->get(currentKeyword_, offset, limit, cachedPapers, cachedTotal)) {
        qDebug() << "Cache HIT! Displaying cached papers for page" << pageNum;

        // Display cached results immediately with correct page number
        resultView_->setPapers(cachedPapers, cachedTotal, pageNum);

        QString message = QString("第 %1 页（来自缓存）- 共 %2 篇论文").arg(pageNum).arg(cachedTotal);
        statusBar()->showMessage(message, 3000);

        return;
    }

    // Cache miss - fetch from backend
    qDebug() << "Cache MISS - fetching from backend API";
    statusBar()->showMessage(QString("正在加载第 %1 页...").arg(pageNum));

    // Call API with new offset and limit
    apiManager_->searchPapers(currentKeyword_, "", "", currentOffset_, currentLimit_);
}

void MainWindow::onPaperSelected(int paperId) {
    // Call API to get paper details
    apiManager_->getPaperDetails(paperId);
    statusBar()->showMessage(QString("正在加载论文 %1 的详细信息...").arg(paperId));
}

void MainWindow::onSearchSuccess(const SearchResult& result) {
    // Hide loading indicator
    auto* progressBar = findChild<QProgressBar*>("searchProgress");
    if (progressBar) progressBar->setVisible(false);

    totalResults_ = result.total;

    qDebug() << "=== Backend Search Results ===";
    qDebug() << "Papers received:" << result.papers.size();
    qDebug() << "Total papers:" << result.total;
    qDebug() << "Offset:" << currentOffset_ << "Limit:" << currentLimit_;

    // Convert Paper results for display
    QList<Paper> papers;
    for (const auto& p : result.papers) {
        Paper paper;
        paper.id = p.id;
        paper.title = p.title;
        paper.journal = p.journalShort.isEmpty()
                       ? p.journalFull
                       : p.journalShort;
        paper.year = p.year;
        paper.level = p.level;
        paper.authors = p.authors;
        paper.doiUrl = p.doiUrl;
        papers.append(paper);
    }

    // Cache the results for this page
    paperCache_->insert(currentKeyword_, currentOffset_, currentLimit_, papers, totalResults_);

    // Save to local database
    if (localDb_ && localDb_->isOpen()) {
        for (const auto& paper : papers) {
            if (!localDb_->existsByTitle(paper.title)) {
                localDb_->savePaper(DbPaper::fromPaper(paper));
            }
        }
    }

    // Save to search history (first page only)
    if (currentOffset_ == 0) {
        auto* searchHistory = findChild<SearchHistory*>();
        if (searchHistory) {
            searchHistory->addSearch(currentKeyword_, result.total);
        }
    }

    // Display results with correct page number
    int pageNum = (currentOffset_ / currentLimit_) + 1;
    resultView_->setPapers(papers, totalResults_, pageNum);

    // Update status bar
    int totalPages = (totalResults_ + currentLimit_ - 1) / currentLimit_;
    QString message = QString("Page %1/%2 - Found %3 papers for \"%4\"")
                         .arg(pageNum).arg(totalPages).arg(result.total).arg(currentKeyword_);
    if (result.durationMs > 0) {
        message += QString(" (%1 ms)").arg(result.durationMs, 0, 'f', 2);
    }
    statusBar()->showMessage(message, 5000);

    // Update Search tab badge with result count
    if (tabWidget_) {
        tabWidget_->setTabText(0, QString("Search (%1)").arg(totalResults_));
    }

    // Log cache statistics
    qDebug() << "Cache statistics for" << currentKeyword_ << ":"
             << "Cached pages:" << paperCache_->getCacheCount(currentKeyword_)
             << "Total cache size:" << paperCache_->getCacheSize();

    ToastWidget::showSuccess(QString("Found %1 results for \"%2\"").arg(totalResults_).arg(currentKeyword_));
    if (notificationCenter_) {
        notificationCenter_->addNotification(Notification::Success, "Search Complete",
            QString("Found %1 results for \"%2\"").arg(totalResults_).arg(currentKeyword_));
    }
}

void MainWindow::onSearchFailed(const QString& error) {
    // Hide loading indicator
    auto* progressBar = findChild<QProgressBar*>("searchProgress");
    if (progressBar) progressBar->setVisible(false);

    // Try local database fallback
    if (localDb_ && localDb_->isOpen()) {
        DbSearchResult localResult = localDb_->searchPapers(currentKeyword_, currentOffset_, currentLimit_);
        if (!localResult.papers.isEmpty()) {
            resultView_->setPapers(localResult.papers, localResult.totalCount, 1);
            statusBar()->showMessage(
                QString("API offline — showing %1 local results for \"%2\"")
                    .arg(localResult.papers.size()).arg(currentKeyword_), 5000);
            return;
        }
    }

    resultView_->clear();
    QMessageBox::warning(this, "Search Failed",
        QString("Search error:\n%1\n\n"
                "Backend may be offline. Start it at http://localhost:8080").arg(error));
    statusBar()->showMessage("Search failed", 3000);
}

void MainWindow::onHealthCheckSuccess(bool healthy, const QString& message) {
    Q_UNUSED(message);
    qint64 latencyMs = healthTimer_.elapsed();

    if (heroWidget_) {
        heroWidget_->setHealthStatus(healthy
            ? HeroWidget::HealthStatus::Healthy
            : HeroWidget::HealthStatus::Unhealthy);
    }

    // Status bar health indicator
    auto* healthLabel = findChild<QLabel*>("healthIndicator");
    if (!healthLabel) {
        healthLabel = new QLabel(this);
        healthLabel->setObjectName("healthIndicator");
        statusBar()->addPermanentWidget(healthLabel);
    }
    healthLabel->setStyleSheet(
        QString("padding: 2px 8px; border-radius: 4px; font-size: 11px; font-weight: bold; "
                "color: %1; background: %2;")
        .arg(healthy ? "#059669" : "#dc2626", healthy ? "#d1fae5" : "#fee2e2")
    );
    healthLabel->setText(healthy ? QString("Online %1ms").arg(latencyMs) : "Offline");

    // Cache stats in status bar
    auto* cacheLabel = findChild<QLabel*>("cacheStats");
    if (!cacheLabel && paperCache_) {
        cacheLabel = new QLabel(this);
        cacheLabel->setObjectName("cacheStats");
        cacheLabel->setStyleSheet("color: palette(mid); font-size: 11px; padding: 2px 6px;");
        statusBar()->addPermanentWidget(cacheLabel);
    }
    if (cacheLabel && paperCache_) {
        cacheLabel->setText(QString("Cache: %1").arg(paperCache_->getCacheSize()));
    }

    // Window title with status
    QString title = healthy ? "PaperCrawler - Connected" : "PaperCrawler - Offline";
    if (authManager_ && authManager_->isAuthenticated()) {
        title += QString(" (%1)").arg(authManager_->getCurrentUser().username);
    }
    setWindowTitle(title);

    // Load recent papers on first health check success
    if (healthy) {
        apiManager_->getRecentPapers(6);
    }
}

void MainWindow::onNetworkError(const QString& error) {
    qWarning() << "Network error:" << error;
    statusBar()->showMessage("网络错误: " + error.left(50) + "...", 5000);
}

void MainWindow::onExport(ExportFormat format) {
    // Export dialog with format selection + source options
    QDialog dlg(this);
    dlg.setWindowTitle("Export Papers");
    dlg.setMinimumWidth(420);
    auto* layout = new QVBoxLayout(&dlg);
    layout->setSpacing(12);

    // Format selection
    auto* fmtLabel = new QLabel("Export Format:");
    fmtLabel->setStyleSheet("font-weight: bold; color: palette(text);");
    layout->addWidget(fmtLabel);

    auto* fmtCombo = new QComboBox();
    fmtCombo->addItems({"CSV", "BibTeX", "JSON", "EndNote", "RIS"});
    fmtCombo->setCurrentIndex(static_cast<int>(format));
    fmtCombo->setStyleSheet("padding: 8px; border: 1px solid palette(mid); border-radius: 6px;");
    layout->addWidget(fmtCombo);

    // Source selection
    auto* srcLabel = new QLabel("Export Source:");
    srcLabel->setStyleSheet("font-weight: bold; color: palette(text);");
    layout->addWidget(srcLabel);

    auto* srcCombo = new QComboBox();
    srcCombo->addItems({"Current Search Results", "All Papers", "Favorites", "Recent Papers"});
    srcCombo->setStyleSheet("padding: 8px; border: 1px solid palette(mid); border-radius: 6px;");
    layout->addWidget(srcCombo);

    // Paper count info
    auto* countLabel = new QLabel("");
    countLabel->setStyleSheet("color: palette(mid); font-size: 11px;");
    int resultCount = resultView_ ? resultView_->paperCount() : 0;
    countLabel->setText(QString("Current results: %1 papers").arg(resultCount));
    layout->addWidget(countLabel);

    // Options
    auto* includeAbstract = new QCheckBox("Include Abstracts");
    includeAbstract->setChecked(true);
    layout->addWidget(includeAbstract);

    auto* includeTags = new QCheckBox("Include Tags");
    includeTags->setChecked(true);
    layout->addWidget(includeTags);

    // Buttons
    auto* btnRow = new QHBoxLayout();
    auto* cancelBtn = new QPushButton("Cancel");
    cancelBtn->setStyleSheet(
        "QPushButton { background: palette(button); color: palette(button-text); "
        "border: 1px solid palette(mid); border-radius: 6px; padding: 8px 20px; }"
    );
    connect(cancelBtn, &QPushButton::clicked, &dlg, &QDialog::reject);
    btnRow->addStretch();
    btnRow->addWidget(cancelBtn);

    auto* exportBtn = new QPushButton("Export");
    exportBtn->setStyleSheet(
        "QPushButton { background: #4f46e5; color: white; border: none; "
        "border-radius: 6px; padding: 8px 24px; font-weight: bold; }"
        "QPushButton:hover { background: #4338ca; }"
    );
    btnRow->addWidget(exportBtn);
    layout->addLayout(btnRow);

    connect(exportBtn, &QPushButton::clicked, this, [this, &dlg, fmtCombo, srcCombo,
                                                       includeAbstract, includeTags, resultCount]() {
        QString fmtStr = fmtCombo->currentText().toLower();
        int srcIdx = srcCombo->currentIndex();

        // Try server-side export first
        QJsonObject params;
        params["format"] = fmtStr;
        params["include_abstract"] = includeAbstract->isChecked();
        params["include_tags"] = includeTags->isChecked();

        if (srcIdx == 0 && resultCount > 0) {
            // Current search results
            QList<Paper> papers = resultView_->getPapers();
            QJsonArray paperIds;
            for (const auto& p : papers) paperIds.append(p.id);
            params["paper_ids"] = paperIds;
            params["query"] = currentKeyword_;
        } else if (srcIdx == 1) {
            params["source"] = "all";
        } else if (srcIdx == 2) {
            params["source"] = "favorites";
        } else if (srcIdx == 3) {
            params["source"] = "recent";
            params["limit"] = 50;
        }

        apiManager_->exportData(params);
        dlg.accept();
        statusBar()->showMessage("Export requested...", 3000);

        // Also do local export as fallback
        if (srcIdx == 0 && resultCount > 0) {
            ExportFormat fmt = static_cast<ExportFormat>(fmtCombo->currentIndex());
            if (fmt < ExportFormat::PDF) {
                QString fileName = exportManager_->showSaveDialog(this, fmt);
                if (!fileName.isEmpty()) {
                    QList<Paper> papers = resultView_->getPapers();
                    bool ok = false;
                    switch (fmt) {
                        case ExportFormat::CSV: ok = exportManager_->exportToCSV(fileName, papers); break;
                        case ExportFormat::BibTeX: ok = exportManager_->exportToBibTeX(fileName, papers); break;
                        case ExportFormat::JSON: ok = exportManager_->exportToJSON(fileName, papers); break;
                        case ExportFormat::PDF: ok = exportManager_->exportToPDF(fileName, papers); break;
                    }
                    if (ok) statusBar()->showMessage(
                        QString("Exported %1 papers to %2").arg(papers.size()).arg(fileName), 5000);
                }
            }
        }
    });

    dlg.exec();
}

void MainWindow::onPreferences() {
    auto* dialog = new SettingsDialog(apiManager_, authManager_, this);
    dialog->exec();
    dialog->deleteLater();
}

void MainWindow::onToggleTheme() {
    if (themeManager_) {
        themeManager_->toggleTheme();

        // Update theme button icon
        QString icon = themeManager_->currentTheme() == ThemeManager::ThemeMode::Dark
                      ? "☀️" : "🌙";
        themeButton_->setText(icon);

        // Update status bar
        QString themeName = themeManager_->currentTheme() == ThemeManager::ThemeMode::Dark
                           ? "Dark" : "Light";
        statusBar()->showMessage(themeName + " theme enabled - Enjoy the modern UI!", 3000);

        // Trigger repaint for gradient update
        update();

        // Update LaTeX editor dark mode
        if (latexEditor_) {
            latexEditor_->setDarkMode(themeManager_->currentTheme() == ThemeManager::ThemeMode::Dark);
        }
    }
}

void MainWindow::onShowStatistics() {
    tabWidget_->setCurrentIndex(4);
    apiManager_->getStats("overview");
}

void MainWindow::onAbout() {
    int favCount = 0;
    auto* favMgr = findChild<FavoriteManager*>();
    if (favMgr) favCount = favMgr->getFavoriteCount();
    int cacheSize = paperCache_ ? paperCache_->getCacheSize() : 0;
    int dbCount = localDb_ ? localDb_->getPaperCount() : 0;

    QMessageBox::about(this, "About PaperCrawler",
        QString(
        "<h2>PaperCrawler Desktop</h2>"
        "<p>Version 2.0.0</p>"
        "<p>Academic paper search, management and analysis tool.</p>"
        "<hr>"
        "<p><b>Features:</b></p>"
        "<ul>"
        "<li>Full-text search with pagination and caching</li>"
        "<li>CCF level / year / type filters</li>"
        "<li>Favorites with notes (%1 saved)</li>"
        "<li>Export to CSV / BibTeX / JSON / PDF</li>"
        "<li>Crawler task management</li>"
        "<li>AI research assistant (chat + review)</li>"
        "<li>Statistics dashboard</li>"
        "<li>Local SQLite database (%2 papers)</li>"
        "<li>Offline search fallback</li>"
        "<li>Dark theme support</li>"
        "<li>System tray integration</li>"
        "</ul>"
        "<hr>"
        "<p>Cache: %3 entries | Server: %4</p>"
        "<p>Built with Qt 6 / C++17</p>"
        "<p>&copy; 2026 PaperCrawler Project</p>"
        ).arg(favCount).arg(dbCount).arg(cacheSize).arg(apiManager_->baseUrl())
    );
}

void MainWindow::onPaperAdded(int paperId) {
    qDebug() << "Paper added to database:" << paperId;
}

void MainWindow::onDatabaseError(const QString& error) {
    qWarning() << "Database error:" << error;
    statusBar()->showMessage("数据库错误: " + error.left(50), 5000);
}

void MainWindow::closeEvent(QCloseEvent* event) {
    // Save session
    if (sessionManager_) {
        sessionManager_->saveSession(currentKeyword_, currentOffset_,
            tabWidget_->currentIndex(), saveGeometry(), saveState());
        if (!currentKeyword_.isEmpty()) {
            sessionManager_->saveRecentSearch(currentKeyword_);
        }
    }

    if (trayManager_ && QSystemTrayIcon::isSystemTrayAvailable()) {
        hide();
        trayManager_->showNotification("PaperCrawler",
            "Running in background. Double-click tray icon to restore.");
        event->ignore();
    } else {
        saveSettings();
        event->accept();
    }
}

// ============================================================================
// Paper Details
// ============================================================================

void MainWindow::onPaperDetailsSuccess(const Paper& paper) {
    auto* favMgr = findChild<FavoriteManager*>();
    auto* dialog = new PaperDetailDialog(paper, favMgr, apiManager_, this);
    connect(dialog, &PaperDetailDialog::favoriteToggled, this, [this](int, bool) {
        if (tabWidget_->currentIndex() == 1) refreshFavoritesTab();
    });
    connect(dialog, &PaperDetailDialog::paperDeleted, this, [this](int paperId) {
        statusBar()->showMessage(QString("Paper %1 deleted").arg(paperId), 5000);
        ToastWidget::showInfo(QString("Paper %1 deleted").arg(paperId));
        if (!currentKeyword_.isEmpty()) {
            apiManager_->searchPapers(currentKeyword_, "", "", currentOffset_, currentLimit_);
        }
    });
    connect(dialog, &PaperDetailDialog::tagsChanged, this, [](const QStringList&) {
        ToastWidget::showSuccess("Tags updated");
    });

    // Record in recent history
    if (recentHistory_) {
        recentHistory_->addEntry(paper.id, paper.title, paper.authors, paper.year);
    }

    dialog->exec();
    dialog->deleteLater();
}

// ============================================================================
// Authentication Integration
// ============================================================================

void MainWindow::initializeAuthentication() {
    authManager_ = new AuthManager(this);
    authManager_->setBaseUrl(apiManager_->baseUrl());

    connect(authManager_, &AuthManager::authenticationChanged,
            this, &MainWindow::onAuthenticationChanged);
    connect(authManager_, &AuthManager::loginSuccess,
            this, &MainWindow::onLoginSuccess);
    connect(authManager_, &AuthManager::logoutSuccess,
            this, &MainWindow::onLogoutSuccess);

    if (authManager_->loadSavedState()) {
        qDebug() << "Restored session for user:" << authManager_->getCurrentUser().username;
        apiManager_->setAuthToken(authManager_->getAccessToken());
    }
}

void MainWindow::showLoginDialog() {
    if (!authManager_) {
        initializeAuthentication();
    }

    loginWindow_ = new LoginWindow(authManager_, this);
    connect(loginWindow_, &LoginWindow::authenticationSuccessful,
            this, &MainWindow::onLoginSuccess);
    loginWindow_->exec();
    loginWindow_->deleteLater();
    loginWindow_ = nullptr;
}

void MainWindow::handleLogout() {
    if (!authManager_) return;

    auto reply = QMessageBox::question(this, "Logout",
        "Are you sure you want to logout?",
        QMessageBox::Yes | QMessageBox::No);

    if (reply == QMessageBox::Yes) {
        authManager_->logout();
    }
}

void MainWindow::onLoginSuccess(const DesktopUser& user) {
    qDebug() << "Login successful:" << user.username;
    apiManager_->setAuthToken(authManager_->getAccessToken());
    updateAuthUI();
    ToastWidget::showSuccess(QString("Welcome, %1!").arg(user.username));
}

void MainWindow::onLogoutSuccess() {
    qDebug() << "Logout successful";
    apiManager_->clearAuthToken();
    updateAuthUI();
    ToastWidget::showInfo("Logged out");
}

void MainWindow::onAuthenticationChanged(bool authenticated) {
    qDebug() << "Auth state changed:" << authenticated;
    if (authenticated) {
        apiManager_->setAuthToken(authManager_->getAccessToken());
    }
    updateAuthUI();
}

void MainWindow::updateAuthUI() {
    if (!authManager_) return;

    bool authenticated = authManager_->isAuthenticated();

    if (loginAction_) loginAction_->setVisible(!authenticated);
    if (logoutAction_) logoutAction_->setVisible(authenticated);
    if (profileAction_) profileAction_->setVisible(authenticated);

    if (userLabel_) {
        if (authenticated) {
            DesktopUser user = authManager_->getCurrentUser();
            userLabel_->setText(QString("%1").arg(user.username));
            userLabel_->setToolTip(QString("Logged in as %1\n%2")
                .arg(user.fullName, user.email));
        } else {
            userLabel_->setText("");
        }
    }

    if (authenticated) {
        DesktopUser user = authManager_->getCurrentUser();
        setWindowTitle(QString("PaperCrawler - %1").arg(user.username));
    } else {
        setWindowTitle("PaperCrawler");
    }
}

// ============================================================================
// Favorites Tab Refresh
// ============================================================================

void MainWindow::refreshFavoritesTab() {
    auto* page = findChild<QWidget*>("favoritesPage");
    if (!page) return;

    auto* favMgr = findChild<FavoriteManager*>();
    if (!favMgr) return;

    auto* countLabel = page->findChild<QLabel*>("favCountLabel");
    auto* listLayout = page->findChild<QVBoxLayout*>("favListLayout");

    if (!listLayout) return;

    // Clear existing items
    QLayoutItem* item;
    while ((item = listLayout->takeAt(0)) != nullptr) {
        delete item->widget();
        delete item;
    }

    auto favorites = favMgr->getFavorites();

    if (countLabel) {
        countLabel->setText(QString("%1 paper(s) saved").arg(favorites.size()));
    }

    // Update tab badge
    if (tabWidget_) {
        tabWidget_->setTabText(1, QString("Favorites (%1)").arg(favorites.size()));
    }

    if (favorites.isEmpty()) {
        auto* emptyLabel = new QLabel("No favorites yet. Click the star on paper cards to add.");
        emptyLabel->setStyleSheet("color: palette(mid); font-size: 14px; padding: 40px;");
        emptyLabel->setAlignment(Qt::AlignCenter);
        listLayout->addWidget(emptyLabel);
        return;
    }

    for (const auto& fav : favorites) {
        auto* card = new QWidget();
        card->setStyleSheet(
            "QWidget { background: palette(base); border: 1px solid palette(mid); "
            "border-radius: 8px; padding: 12px 16px; }"
        );

        auto* cardLayout = new QVBoxLayout(card);
        cardLayout->setContentsMargins(0, 0, 0, 0);
        cardLayout->setSpacing(4);

        auto* titleLabel = new QLabel(fav.title);
        titleLabel->setWordWrap(true);
        titleLabel->setStyleSheet("font-weight: bold; color: palette(text); font-size: 14px;");
        cardLayout->addWidget(titleLabel);

        // Notes display
        if (!fav.notes.isEmpty()) {
            auto* notesLabel = new QLabel(fav.notes.left(100));
            notesLabel->setWordWrap(true);
            notesLabel->setStyleSheet("color: palette(mid); font-size: 12px; font-style: italic; padding-left: 8px; border-left: 2px solid #e0e7ff;");
            cardLayout->addWidget(notesLabel);
        }

        auto* metaRow = new QHBoxLayout();
        auto* journalLabel = new QLabel(fav.journal);
        journalLabel->setStyleSheet("color: palette(mid); font-size: 12px;");
        auto* yearLabel = new QLabel(fav.year);
        yearLabel->setStyleSheet("color: palette(mid); font-size: 12px;");
        metaRow->addWidget(journalLabel);
        metaRow->addWidget(yearLabel);
        metaRow->addStretch();

        // Note button
        auto* noteBtn = new QPushButton("Note");
        noteBtn->setStyleSheet(
            "QPushButton { background: none; border: 1px solid palette(mid); color: palette(text); "
            "border-radius: 4px; padding: 2px 8px; font-size: 11px; }"
            "QPushButton:hover { background: #e0e7ff; }"
        );
        int pid = fav.paperId;
        connect(noteBtn, &QPushButton::clicked, this, [this, favMgr, pid]() {
            QString currentNote = favMgr->getNotes(pid);
            bool ok;
            QString note = QInputDialog::getMultiLineText(this, "Edit Note",
                "Add notes for this paper:", currentNote, &ok);
            if (ok) {
                favMgr->setNotes(pid, note);
                refreshFavoritesTab();
            }
        });
        metaRow->addWidget(noteBtn);

        auto* removeBtn = new QPushButton("Remove");
        removeBtn->setStyleSheet(
            "QPushButton { background: none; border: none; color: #ef4444; "
            "font-size: 11px; font-weight: bold; }"
            "QPushButton:hover { color: #dc2626; }"
        );
        connect(removeBtn, &QPushButton::clicked, this, [this, favMgr, pid]() {
            favMgr->removeFavorite(pid);
        });
        metaRow->addWidget(removeBtn);
        cardLayout->addLayout(metaRow);

        listLayout->addWidget(card);
    }
}
