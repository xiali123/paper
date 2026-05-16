#!/bin/bash
# Route definitions for AiCoPilot module
# Add new routes: append to ROUTES array
# Format: "METHOD|/path|body_json|expected_codes|test_name"

MODULE_NAME="AiCoPilot"
ROUTES=(
    "POST|/api/ai-co-pilot/review|{\"paperId\":1}|200,404|Submit paper for AI review"
    "POST|/api/ai-co-pilot/literature-review/generate|{\"topic\":\"machine learning\"}|200|Generate literature review"
    "POST|/api/ai-co-pilot/research-plan/generate|{\"topic\":\"deep learning\"}|200|Generate research plan"
    "POST|/api/ai-co-pilot/chat|{\"message\":\"hello\"}|200|Send chat message"
    "GET|/api/ai-co-pilot/reviews/1||200,404|Get AI review by ID"
    "GET|/api/ai-co-pilot/literature-reviews||200|List all literature reviews"
    "GET|/api/ai-co-pilot/research-plans||200|List all research plans"
    "GET|/api/ai-co-pilot/conversations||200|List all conversations"
    "GET|/api/ai-co-pilot/recommendations||200|Get AI recommendations"
    "GET|/api/ai-co-pilot/stats||200|Get AI co-pilot stats"
    "GET|/api/ai-co-pilot/costs||200|Get AI co-pilot costs"
    "GET|/api/ai-co-pilot/reviews||200|Review history"
    "GET|/api/ai-co-pilot/literature-reviews||200|Literature review history"
    "GET|/api/ai-co-pilot/plans||200|Research plan history"
    "GET|/api/ai-co-pilot/sessions/1/messages||200|Get session messages"
    "POST|/api/ai-co-pilot/regenerate/1||200|Regenerate review"
    "POST|/api/ai-co-pilot/feedback|{\"type\":\"review\",\"rating\":5,\"comment\":\"Good\"}|200|Submit AI feedback"
    "POST|/api/ai-co-pilot/sessions/1/rename|{\"name\":\"New Session Name\"}|200|Rename session"
    "GET|/api/ai-co-pilot/sessions/recent||200|Get recent sessions"
    "POST|/api/ai-co-pilot/export|{\"sessionId\":\"session_001\",\"format\":\"markdown\"}|200|Export conversation"

    # --- v5 Additions ---
    "POST|/api/ai-co-pilot/sessions/1/pin|{\"pinned\":true}|200|Pin/unpin a session"
    "GET|/api/ai-co-pilot/sessions/pinned||200|Get pinned sessions"
    "POST|/api/ai-co-pilot/sessions/1/summarize||200|Summarize a conversation session"

    # --- v6 Additions ---
    "POST|/api/ai-co-pilot/sessions/1/bookmark|{\"messageId\":\"1\",\"bookmarked\":true}|200|Bookmark/unbookmark a message"
    "GET|/api/ai-co-pilot/bookmarks||200|Get all bookmarked messages"
    "POST|/api/ai-co-pilot/sessions/merge|{\"sessionIds\":[\"s1\",\"s2\"],\"newName\":\"Merged\"}|200|Merge multiple sessions"

    # --- Round 21 Additions ---
    "GET|/api/ai-co-pilot/sessions/1/context||200,404|Get session context"
    "POST|/api/ai-co-pilot/sessions/1/clear|{}|200|Clear session messages"
    "GET|/api/ai-co-pilot/sessions/search?q=test||200|Search across sessions"

    # --- Round 25 Additions ---
    "POST|/api/ai-co-pilot/analyze|{\"text\":\"This is text\",\"type\":\"grammar\"}|200|Analyze text for writing"
    "GET|/api/ai-co-pilot/sessions/stats||200|Get AI CoPilot usage stats"
    "POST|/api/ai-co-pilot/prompts/custom|{\"name\":\"My Prompt\",\"template\":\"Summarize\"}|200|Save custom prompt"

    # --- Round 28 Additions ---
    "POST|/api/ai-co-pilot/sessions/1/export|{\"format\":\"markdown\"}|200|Export session conversation"
    "GET|/api/ai-co-pilot/prompts/popular||200|Get popular prompt templates"

    # --- Round 30 Additions ---
    "POST|/api/ai-co-pilot/sessions/1/rename|{\"name\":\"New Name\"}|200|Rename session"
    "GET|/api/ai-co-pilot/health||200|Get AI CoPilot health"

    # --- Round 32 Additions ---
    "POST|/api/ai-co-pilot/text/improve|{\"text\":\"test\",\"improvements\":[\"clarity\"]}|200|Improve text quality"
    "GET|/api/ai-co-pilot/sessions/export||200|Export all sessions"

    # --- Round 33 Additions ---
    "POST|/api/ai-co-pilot/templates/apply|{\"sessionId\":1,\"templateId\":3,\"variables\":{}}|200|Apply prompt template"
    "GET|/api/ai-co-pilot/templates?category=review||200|List prompt templates"

    # --- Round 34 Additions ---
    "POST|/api/ai-co-pilot/batch/analyze|{\"paperIds\":[1,2,3],\"analysisType\":\"summary\"}|200|Batch analyze papers"
    "GET|/api/ai-co-pilot/sessions/1/timeline||200|Get session timeline"

    # --- Round 35 Additions ---
    "DELETE|/api/ai-co-pilot/sessions/1||200|Delete AI session"
    "GET|/api/ai-co-pilot/costs/breakdown?period=month||200|Get cost breakdown"

    # --- Round 36 Additions ---
    "PUT|/api/ai-co-pilot/sessions/1/settings|{\"model\":\"gpt-4\",\"temperature\":0.7}|200|Update session settings"
    "GET|/api/ai-co-pilot/usage/daily?date=2024-01-15||200|Get daily AI usage"

    # --- Round 37 Additions ---
    "POST|/api/ai-co-pilot/compare|{\"paperIds\":[1,2],\"aspects\":[\"methodology\"]}|200|Compare papers"
    "GET|/api/ai-co-pilot/models/comparison||200|Compare AI models"

    # --- Round 38 Additions ---
    "POST|/api/ai-co-pilot/summarize/batch|{\"texts\":[\"text1\",\"text2\"],\"maxLength\":200}|200|Batch summarize texts"
    "GET|/api/ai-co-pilot/sessions/1/export/pdf||200|Export session as PDF"

    # --- Round 39 Additions ---
    "POST|/api/ai-co-pilot/translate|{\"text\":\"Hello world\",\"sourceLang\":\"en\",\"targetLang\":\"zh\"}|200|Translate text"
    "GET|/api/ai-co-pilot/sessions/1/summary||200|Get session summary"

    # --- Round 40 Additions ---
    "POST|/api/ai-co-pilot/paraphrase|{\"text\":\"The results show\",\"style\":\"academic\"}|200|Paraphrase text"
    "GET|/api/ai-co-pilot/prompts/categories||200|List prompt categories"

    # --- Round 41 Additions ---
    "POST|/api/ai-co-pilot/outline/generate|{\"topic\":\"ML in Healthcare\",\"sections\":5}|200|Generate outline"
    "GET|/api/ai-co-pilot/usage/monthly?year=2024&month=1||200|Get monthly usage"

    # --- Round 42 Additions ---
    "POST|/api/ai-co-pilot/keywords/extract|{\"text\":\"Machine learning healthcare\",\"maxKeywords\":10}|200|Extract keywords"
    "GET|/api/ai-co-pilot/outlines/history?limit=10||200|Get outline history"

    # --- Round 43 Additions ---
    "POST|/api/ai-co-pilot/grammar/check|{\"text\":\"This are a test\",\"language\":\"en\"}|200|Check grammar"
    "GET|/api/ai-co-pilot/sessions/1/messages/search?q=machine||200|Search session messages"

    # --- Round 44 Additions ---
    "POST|/api/ai-co-pilot/abstract/generate|{\"content\":\"Paper content\",\"type\":\"structured\",\"maxLength\":250}|200|Generate abstract"
    "GET|/api/ai-co-pilot/sessions/stats/aggregate?period=month||200|Get aggregate session stats"

    # --- Round 45 Additions ---
    "POST|/api/ai-co-pilot/citation/format|{\"citations\":[{\"title\":\"Test\"}],\"style\":\"apa\"}|200|Format citations"
    "GET|/api/ai-co-pilot/sessions/1/context/window?messageId=10||200|Get context window"

    # --- Round 46 Additions ---
    "POST|/api/ai-co-pilot/embedding/generate|{\"text\":\"test embedding\",\"model\":\"text-embedding-ada-002\"}|200|Generate embedding"
    "GET|/api/ai-co-pilot/usage/by-model?period=month||200|Get usage by model"

    # --- Round 47 Additions ---
    "POST|/api/ai-co-pilot/table/extract|{\"text\":\"Name Age\\nJohn 30\",\"format\":\"json\"}|200|Extract table data"
    "GET|/api/ai-co-pilot/outlines/1||200|Get outline by ID"

    # --- Round 48 Additions ---
    "POST|/api/ai-co-pilot/paraphrase/batch|{\"texts\":[\"text1\",\"text2\"],\"style\":\"academic\"}|200|Batch paraphrase multiple texts"
    "GET|/api/ai-co-pilot/models/comparison||200|Compare AI models with metrics"

    # --- Round 49 Additions ---
    "POST|/api/ai-co-pilot/citations/format|{\"citations\":[{\"title\":\"Test Paper\",\"authors\":\"Smith\",\"year\":\"2024\",\"journal\":\"Nature\"}],\"style\":\"apa\"}|200|Format citations from a list"
    "GET|/api/ai-co-pilot/usage/summary?period=month||200|Get AI usage summary"

    # --- Round 50 Additions ---
    "GET|/api/ai-co-pilot/prompts/library?category=all||200|Get prompt library"
    "GET|/api/ai-co-pilot/grammar/rules?language=en||200|Get grammar rules reference"

    # --- Round 51 Additions ---
    "POST|/api/ai-co-pilot/style/analyze|{\"text\":\"This is a sample academic text for style analysis.\"}|200|Analyze writing style"
    "GET|/api/ai-co-pilot/usage/by-date?startDate=2026-05-05&endDate=2026-05-12||200|Get usage stats grouped by date"

    # --- Round 52 Additions ---
    "POST|/api/ai-co-pilot/paper/summarize|{\"paperId\":\"paper_123\",\"maxLength\":500}|200|Summarize a paper by ID"
    "GET|/api/ai-co-pilot/recommendations/personalized?userId=user_42||200|Get personalized AI recommendations"

    # --- Round 53 Additions ---
    "POST|/api/ai-co-pilot/document/compare|{\"doc1Id\":\"doc_1\",\"doc2Id\":\"doc_2\",\"aspects\":[\"methodology\"]}|200|Compare two documents"
    "GET|/api/ai-co-pilot/models/performance?model=gpt-4||200|Get model performance metrics"

    # --- Round 54 Additions ---
    "POST|/api/ai-co-pilot/plagiarism/check|{\"text\":\"This is a novel approach to deep learning\",\"sensitivity\":0.8}|200|Check text for plagiarism"
    "GET|/api/ai-co-pilot/sessions/1/analytics?period=7d||200|Get session analytics"
    "POST|/api/ai-co-pilot/sentiment/analyze|{\"text\":\"The results demonstrate significant improvement\",\"language\":\"en\"}|200|Analyze sentiment of text"
    "GET|/api/ai-co-pilot/sessions/1/export/status?taskId=export_001||200|Get export task status"

    # --- Round 55 Additions ---
    "POST|/api/ai-co-pilot/code/generate|{\"description\":\"Sort a list\",\"language\":\"python\"}|200|Generate code snippet from description"
    "GET|/api/ai-co-pilot/feedback/summary||200|Get feedback summary statistics"

    # --- Round 56 Additions ---
    "POST|/api/ai-co-pilot/tone/adjust|{\"text\":\"The results are good\",\"tone\":\"formal\"}|200|Adjust text tone"
    "GET|/api/ai-co-pilot/sessions/1/versions||200|Get session version history"

    # --- Round 57 Additions ---
    "POST|/api/ai-co-pilot/writing-assist/suggest|{\"text\":\"The results are good\",\"mode\":\"academic\"}|200|Get writing assistance suggestions"
    "GET|/api/ai-co-pilot/knowledge-graph/entities||200|Get knowledge graph entities"

    # --- Round 58 Additions ---
    "POST|/api/ai-co-pilot/sessions/1/share|{\"targetUser\":\"user_42\",\"permission\":\"read\"}|200|Share session with another user"
    "GET|/api/ai-co-pilot/quotas/status||200|Get AI usage quotas and remaining limits"

    # --- Round 59 Additions ---
    "POST|/api/ai-co-pilot/abstract/score|{\"abstract\":\"This paper presents a novel approach.\"}|200|Score abstract for quality metrics"
    "GET|/api/ai-co-pilot/models/defaults||200|Get default AI model configurations"

    # --- Round 60 Additions ---
    "POST|/api/ai-co-pilot/references/suggest|{\"topic\":\"transformer architecture\",\"maxResults\":5}|200|Suggest academic references for a topic"
    "GET|/api/ai-co-pilot/tasks/history?limit=10||200|Get background task execution history"

    # --- Round 61 Additions ---
    "POST|/api/ai-co-pilot/hypothesis/generate|{\"topic\":\"deep learning\",\"maxHypotheses\":5,\"field\":\"computer science\"}|200|Generate research hypotheses"
    "GET|/api/ai-co-pilot/sessions/1/export/formats||200|Get available export formats"

    # --- Round 62 Additions ---
    "POST|/api/ai-co-pilot/methodology/validate|{\"methodology\":\"randomized controlled trial\",\"researchType\":\"experimental\",\"sampleSize\":25}|200|Validate research methodology design"
    "GET|/api/ai-co-pilot/prompts/recent?limit=10||200|Get recently used prompts"

    # --- Round 63 Additions ---
    "POST|/api/ai-co-pilot/dataset/recommend|{\"topic\":\"machine learning\",\"maxResults\":5}|200|Recommend datasets for a research topic"
    "GET|/api/ai-co-pilot/sessions/1/metadata||200|Get session metadata"

    # --- Round 64 Additions ---
    "POST|/api/ai-co-pilot/figure/describe|{\"figureType\":\"chart\",\"context\":\"experimental results\"}|200|Generate figure description from image data"
    "GET|/api/ai-co-pilot/sessions/1/notes||200|Get session notes and annotations"

    # --- Round 65 Additions ---
    "POST|/api/ai-co-pilot/equation/convert|{\"description\":\"E equals m c squared\",\"format\":\"latex\"}|200|Convert natural language to LaTeX equation"
    "GET|/api/ai-co-pilot/sidebar/config||200|Get sidebar widget configuration"

    # --- Round 66 Additions ---
    "POST|/api/ai-co-pilot/toc/generate|{\"content\":\"Introduction... Methodology... Results...\",\"maxDepth\":3}|200|Generate table of contents from content"
    "GET|/api/ai-co-pilot/workspace/recent?limit=10||200|Get recent AI workspace activities"

    # --- Round 67 Additions ---
    "POST|/api/ai-co-pilot/vocabulary/enrich|{\"text\":\"This shows good results\",\"level\":\"intermediate\",\"maxSuggestions\":5}|200|Enrich vocabulary for academic text"
    "GET|/api/ai-co-pilot/sessions/1/annotations?page=1&pageSize=20||200|Get session annotations"

    # --- Round 68 Additions ---
    "POST|/api/ai-co-pilot/draft/improve|{\"content\":\"This paper shows results\",\"section\":\"introduction\",\"improvementType\":\"clarity\"}|200|Improve a draft section with AI suggestions"
    "GET|/api/ai-co-pilot/draft/templates?category=all||200|Get draft templates for paper sections"

    # --- Round 69 Additions ---
    "POST|/api/ai-co-pilot/conclusion/generate|{\"paperId\":\"paper_456\",\"abstract\":\"Study on ML\",\"keyFindings\":\"Improved accuracy\",\"maxLength\":500}|200|Generate a conclusion section for a paper"
    "GET|/api/ai-co-pilot/conclusion/templates?category=all||200|Get conclusion section templates"

    # --- Round 70 Additions ---
    "POST|/api/ai-co-pilot/title/suggest|{\"content\":\"deep learning for NLP\",\"field\":\"computer science\",\"maxSuggestions\":5}|200|Suggest paper titles based on content"
    "GET|/api/ai-co-pilot/models/availability?provider=all||200|Check available AI models and their status"

    # --- Round 71 Additions ---
    "POST|/api/ai-co-pilot/literature-map/generate|{\"topic\":\"transformer architectures\",\"maxNodes\":20}|200|Generate a literature map visualization"
    "GET|/api/ai-co-pilot/research-trends?field=computer+science&years=5||200|Get research trend analysis"

    # --- Round 72 Additions ---
    "POST|/api/ai-co-pilot/writing-mode/set|{\"mode\":\"academic\",\"sessionId\":\"session_1\",\"language\":\"en\"}|200|Set active writing mode for AI assistance"
    "GET|/api/ai-co-pilot/writing-mode/modes||200|Get available writing modes and current settings"

    # --- Round 73 Additions ---
    "POST|/api/ai-co-pilot/draft/outline|{\"content\":\"Introduction... Methodology...\",\"maxSections\":10,\"style\":\"academic\"}|200|Generate structured outline from draft content"
    "GET|/api/ai-co-pilot/sessions/favorites?limit=20&offset=0||200|Get favorited sessions"

    # --- Round 74 Additions ---
    "POST|/api/ai-co-pilot/threat-assess|{\"content\":\"Paper draft content\",\"assessmentType\":\"comprehensive\",\"severityThreshold\":3}|200|Assess threats in paper draft"
    "GET|/api/ai-co-pilot/sessions/1/performance?period=7d||200|Get session performance metrics"

    # --- Round 75 Additions ---
    "POST|/api/ai-co-pilot/methodology/compare|{\"methodology1\":\"qualitative\",\"methodology2\":\"quantitative\",\"field\":\"social science\"}|200|Compare two research methodologies"
    "GET|/api/ai-co-pilot/prompts/featured?category=all&limit=10||200|Get featured prompt templates"

    # --- Round 76 Additions ---
    "POST|/api/ai-co-pilot/conflict/detect|{\"paperId\":\"paper_789\",\"conflictType\":\"citation\"}|200|Detect conflicts in paper citations"
    "GET|/api/ai-co-pilot/sessions/1/insights?period=7d||200|Get AI-generated insights for a session"

    # --- Round 77 Additions ---
    "POST|/api/ai-co-pilot/research-gap/identify|{\"topic\":\"transformer architectures\",\"field\":\"computer science\",\"maxGaps\":5}|200|Identify research gaps from topic and literature"
    "GET|/api/ai-co-pilot/sessions/1/sentiment-timeline?granularity=message||200|Get sentiment timeline for a session"

    # --- Round 78 Additions ---
    "POST|/api/ai-co-pilot/related-work/suggest|{\"topic\":\"transformer architectures\",\"field\":\"computer science\",\"maxSuggestions\":5}|200|Suggest related work for a paper topic"
    "GET|/api/ai-co-pilot/sessions/1/ai-profile?period=all||200|Get AI interaction profile for a session"

    # --- Round 79 Additions ---
    "POST|/api/ai-co-pilot/research-question/refine|{\"question\":\"How does deep learning improve medical diagnosis\",\"field\":\"medicine\",\"maxRefinements\":3}|200|Refine a research question with AI"
    "GET|/api/ai-co-pilot/sessions/1/activity-log?page=1&pageSize=20&type=all||200|Get session activity log"

    # --- Round 80 Additions ---
    "POST|/api/ai-co-pilot/notation/convert|{\"notation\":\"\\\\sum_{i=1}^{n} x_i^2\",\"inputFormat\":\"latex\",\"outputFormat\":\"mathml\"}|200|Convert mathematical notation between formats"
    "GET|/api/ai-co-pilot/interaction/heatmap?period=7d&granularity=day||200|Get AI interaction heatmap data"

    # --- Round 81 Additions ---
    "POST|/api/ai-co-pilot/ethics/evaluate|{\"content\":\"This paper discusses facial recognition\",\"paperId\":\"paper_eth_001\",\"framework\":\"general\"}|200|Evaluate paper for ethical concerns"
    "GET|/api/ai-co-pilot/sessions/1/snapshot?version=latest&includeMetadata=true||200|Get point-in-time session snapshot"

    # --- Round 82 Additions ---
    "POST|/api/ai-co-pilot/peer-review/simulate|{\"paperId\":\"paper_rev_001\",\"reviewerCount\":3,\"focusAreas\":[\"methodology\",\"novelty\"]}|200|Simulate AI peer review"
    "GET|/api/ai-co-pilot/sessions/1/export/chunks?chunkSize=5&format=json||200|Get session export chunks"

    # --- Round 83 Additions ---
    "POST|/api/ai-co-pilot/counter-arguments/generate|{\"claim\":\"Deep learning outperforms all traditional methods\",\"field\":\"computer science\",\"maxArguments\":3}|200|Generate counter-arguments for a claim"
    "GET|/api/ai-co-pilot/style/history?limit=10||200|Get AI writing style analysis history"

    # --- Round 84 Additions ---
    "POST|/api/ai-co-pilot/argument/strengthen|{\"argument\":\"Deep learning improves accuracy\",\"section\":\"discussion\",\"maxSuggestions\":5}|200|Strengthen argument with AI suggestions"
    "GET|/api/ai-co-pilot/sessions/1/summary/brief?detail=medium||200|Get brief session summary with key topics"

    # --- Round 85 Additions ---
    "POST|/api/ai-co-pilot/rebuttal/generate|{\"reviewerComment\":\"The methodology lacks proper controls\",\"paperSection\":\"discussion\",\"maxPoints\":5}|200|Generate rebuttal for reviewer comments"
    "GET|/api/ai-co-pilot/writing/streak?period=7d||200|Get writing streak and productivity metrics"

    # --- Round 86 Additions ---
    "POST|/api/ai-co-pilot/coherence/analyze|{\"paperId\":\"paper_123\",\"sections\":[\"introduction\",\"methodology\",\"results\"],\"deepAnalysis\":true}|200|Analyze argument coherence across paper sections"
    "GET|/api/ai-co-pilot/models/tuning-suggestions?model=gpt-4&taskType=analytical||200|Get AI model tuning suggestions"

    # --- Round 87 Additions ---
    "POST|/api/ai-co-pilot/visualize/suggest|{\"dataDescription\":\"Accuracy scores across 5 models\",\"dataType\":\"tabular\",\"numVariables\":3,\"goal\":\"comparison\"}|200|Suggest visualization types for dataset"
    "GET|/api/ai-co-pilot/researcher/profile?userId=user_42&period=month||200|Get AI-assisted researcher profile insights"

    # --- Round 88 Additions ---
    "POST|/api/ai-co-pilot/citation-network/map|{\"seedPaperId\":\"paper_seed_001\",\"maxDepth\":2,\"maxNodes\":50,\"direction\":\"both\"}|200|Build citation network graph from seed paper"
    "GET|/api/ai-co-pilot/feedback/trends?period=month&category=all||200|Get feedback trend analytics over time"

    # --- Round 89 Additions ---
    "POST|/api/ai-co-pilot/debate/prepare|{\"topic\":\"deep learning in healthcare\",\"stance\":\"support\",\"maxPoints\":5}|200|Prepare academic debate position"
    "GET|/api/ai-co-pilot/creativity/score?paperId=paper_123&model=composite||200|Score paper creativity and novelty index"

    # --- Round 90 Additions ---
    "POST|/api/ai-co-pilot/research-narrative/generate|{\"findings\":\"Significant improvement in accuracy\",\"audience\":\"academic\",\"tone\":\"formal\",\"maxLength\":1000}|200|Generate research narrative from findings"
    "GET|/api/ai-co-pilot/collaboration/suggestions?researchField=computer+science&maxResults=5||200|Get collaboration partner suggestions"

    # --- Round 91 Additions ---
    "POST|/api/ai-co-pilot/mind-map/generate|{\"topic\":\"transformer architectures\",\"maxNodes\":15,\"layout\":\"radial\"}|200|Generate AI-powered mind map from research topic"
    "GET|/api/ai-co-pilot/reading-list/smart?researchField=computer+science&limit=10&priority=relevance||200|Get AI-curated smart reading list"

    # --- Round 92 Additions ---
    "POST|/api/ai-co-pilot/research-gap/identify|{\"domain\":\"machine learning\",\"subfield\":\"reinforcement learning\",\"maxGaps\":5}|200|Identify research gaps"
    "GET|/api/ai-co-pilot/impact-prediction?paperId=paper_123&model=ensemble||200|Predict paper impact"

    # --- Round 93 Additions ---
    "POST|/api/ai-co-pilot/literature/synthesize|{\"topic\":\"transformer architectures\",\"sources\":[\"arxiv\",\"semantic_scholar\"],\"maxPapers\":20,\"summaryLength\":\"medium\"}|200|Synthesize literature review"
    "GET|/api/ai-co-pilot/writing/progress?documentId=doc_123&granularity=weekly&days=30||200|Get writing progress metrics"

    # --- Round 94 Additions ---
    "POST|/api/ai-co-pilot/argument/map|{\"claim\":\"Transformers are superior to RNNs\",\"maxSupportPoints\":5,\"includeCounter\":true}|200|Map argument structure"
    "GET|/api/ai-co-pilot/paper/complexity-score?paperId=paper_123&model=standard||200|Score paper complexity"

    # --- Round 95 Additions ---
    "POST|/api/ai-co-pilot/research-question/refine|{\"question\":\"How does attention work?\",\"field\":\"NLP\",\"refinementLevel\":3}|200|Refine research question"
    "GET|/api/ai-co-pilot/session/insights?sessionId=session_123&includeMetrics=true||200|Get session insights"

    # --- Round 96 Additions ---
    "POST|/api/ai-co-pilot/hypothesis/test|{\"hypothesis\":\"test\"}|200,500|hypothesis_test"
    "GET|/api/ai-co-pilot/trending-topics||200|trending_topics"

    # --- Round 97 Additions ---
    "GET|/api/ai-co-pilot/literature/stats||200|literature_stats"
    "POST|/api/ai-co-pilot/summarize/abstract|{\"text\":\"sample abstract\"}|200,500|summarize_abstract"

    # --- Round 98 Additions ---
    "GET|/api/ai-co-pilot/sentiment/paper?paperId=paper_123||200|sentiment_paper"
    "POST|/api/ai-co-pilot/knowledge-graph/build|{\"concepts\":[\"machine learning\",\"deep learning\",\"neural networks\"]}|200|knowledge_graph_build"

    # --- Round 99 Additions ---
    "GET|/api/ai-co-pilot/methodology/compare?method1=qualitative&method2=quantitative||200|Compare research methodologies"
    "POST|/api/ai-co-pilot/experiment/design|{\"researchQuestion\":\"Does deep learning improve accuracy\",\"variables\":[\"learning_rate\",\"batch_size\"]}|200|Design experiment plan"

    # --- Round 100 Additions ---
    "GET|/api/ai-co-pilot/citation/style-check?paperId=paper_123&style=apa||200|Check citation formatting style"
    "POST|/api/ai-co-pilot/abstract/generate|{\"title\":\"Deep Learning for NLP\",\"keywords\":[\"transformers\",\"attention\"],\"keyFindings\":\"Improved accuracy by 15%\"}|200|Generate abstract from paper content"

    # --- Round 101 Additions ---
    "GET|/api/ai-co-pilot/research-trends/visualize?field=computer+science&years=5||200|Visualize research trends over time"
    "POST|/api/ai-co-pilot/paper/rate|{\"paperId\":\"paper_123\",\"ratings\":{\"novelty\":8,\"methodology\":7,\"clarity\":9}}|200|Rate a paper with multiple criteria"

    # --- Round 102 Additions ---
    "GET|/api/ai-co-pilot/conference/deadlines?field=machine+learning||200|Get upcoming conference deadlines"
    "POST|/api/ai-co-pilot/annotation/create|{\"paperId\":\"paper_123\",\"sectionId\":\"sec_1\",\"text\":\"Important finding\",\"annotationType\":\"highlight\"}|200|Create an annotation on a paper section"

    # --- Round 103 Additions ---
    "GET|/api/ai-co-pilot/dataset/recommend?topic=machine+learning&limit=5||200|Recommend datasets for research"
    "POST|/api/ai-co-pilot/workflow/create|{\"name\":\"ML Research\",\"steps\":[\"literature review\",\"data collection\"],\"description\":\"End-to-end ML workflow\"}|200|Create a research workflow"

    # --- Round 104 Additions ---
    "GET|/api/ai-co-pilot/collaboration/find?expertise=machine+learning&limit=5||200|Find potential collaborators"
    "POST|/api/ai-co-pilot/note/smart-create|{\"title\":\"My Research Note\",\"content\":\"This is a research note about machine learning.\",\"tags\":[\"ml\",\"research\"]}|200|Smart create a research note"

    # --- Route 190-191 Additions ---
    "GET|/api/ai-co-pilot/institution/search?query=MIT&country=US||200|Search research institutions"
    "POST|/api/ai-co-pilot/codebook/create|{\"name\":\"Qualitative Codes\",\"codes\":[\"theme_a\",\"theme_b\"],\"description\":\"Codebook for interview analysis\"}|200|Create a qualitative coding codebook"

    # --- Route 192-193 Additions ---
    "GET|/api/ai-co-pilot/survey/suggest?topic=machine+learning||200|Suggest survey questions for a research topic"
    "POST|/api/ai-co-pilot/timeline/create|{\"title\":\"ML Research Timeline\",\"events\":[{\"date\":\"2024-01\",\"label\":\"Literature Review\"}],\"description\":\"Timeline for ML project\"}|200|Create a research timeline"

    # --- Route 194-195 Additions ---
    "GET|/api/ai-co-pilot/methodology/recommend?topic=machine+learning&researchType=experimental||200|Recommend research methodology for a topic"
    "POST|/api/ai-co-pilot/variable/identify|{\"description\":\"Study on deep learning accuracy\",\"variableType\":\"all\"}|200|Identify research variables from a description"

    # --- Route 196-197 Additions ---
    "GET|/api/ai-co-pilot/coauthor/suggest?paperTitle=Deep+Learning+for+NLP&field=computer+science||200|Suggest potential co-authors"
    "POST|/api/ai-co-pilot/concept/map|{\"text\":\"Machine learning improves medical diagnosis\",\"maxConcepts\":10}|200|Create a concept map from research text"

    # --- Route 198-199 Additions ---
    "GET|/api/ai-co-pilot/question/generate?topic=machine+learning&count=5||200|Generate research questions from a topic"
    "POST|/api/ai-co-pilot/protocol/design|{\"title\":\"Deep Learning Study\",\"objectives\":[\"Improve accuracy\",\"Reduce training time\"],\"methodology\":\"experimental\"}|200|Design a research protocol"
)
