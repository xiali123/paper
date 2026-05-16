#!/bin/bash
# Route definitions for RecommendationApi module
# Add new routes: append to ROUTES array
# Format: "METHOD|/path|body_json|expected_codes|test_name"

MODULE_NAME="RecommendationApi"
ROUTES=(
    "GET|/api/recommendations/papers||200|Get paper recommendations"
    "GET|/api/recommendations/trending||200|Get trending recommendations"
    "GET|/api/recommendations/similar/1||200,404|Get similar papers"
    "GET|/api/recommendations/explain/1||200,404|Get recommendation explanation"
    "GET|/api/recommendations/stats||200|Get recommendation stats"
    "POST|/api/recommendations/feedback|{\"paperId\":1,\"rating\":5}|200,400|Submit recommendation feedback"
    "GET|/api/recommendations/profile/1||200|User recommendation profile"
    "GET|/api/recommendations/collaborators/1||200|Get collaborator recommendations"
    "POST|/api/recommendations/batch|{\"paper_ids\":[1,2],\"limit\":5}|200|Batch recommendations"
    "GET|/api/recommendations/feedback/history/1||200|Get feedback history"
    "POST|/api/recommendations/feedback|{\"user_id\":1,\"paper_id\":1,\"type\":\"like\"}|200,201|Submit feedback"
    "GET|/api/recommendations/feedback/1||200|User feedback list"
    "GET|/api/recommendations/personalized/1||200|Personalized recommendations"
    "POST|/api/recommendations/refresh||200|Refresh recommendation cache"
    "DELETE|/api/recommendations/feedback/1||200|Delete feedback"
    "GET|/api/recommendation/history|{\"userId\":\"1\"}|200|Get user recommendation history"
    "POST|/api/recommendation/ignore|{\"recommendationId\":\"1\",\"userId\":\"1\"}|200|Ignore a recommendation"
    "GET|/api/recommendation/categories||200|Get recommendation categories"
    "POST|/api/recommendation/preference|{\"userId\":\"1\",\"categories\":[\"ml\",\"nlp\"],\"minScore\":0.5}|200|Set recommendation preferences"
    "GET|/api/recommendation/similar/1||200|Get similar papers by paper id"
    "POST|/api/recommendation/blocklist|{\"userId\":\"1\",\"paperId\":\"2\"}|200|Add paper to blocklist"
    "GET|/api/recommendation/blocklist|{\"userId\":\"1\"}|200|Get user blocklist"
    "GET|/api/recommendation/trending||200|Get trending papers (weighted score)"
    "POST|/api/recommendation/feedback|{\"userId\":1,\"paperId\":1,\"rating\":5,\"feedback\":\"helpful\"}|200|Submit recommendation feedback"
    "GET|/api/recommendation/stats||200|Get recommendation system statistics"
    "GET|/api/recommendation/recently-viewed||200|Get recently viewed papers"
    "POST|/api/recommendation/collaborative|{\"userId\":\"1\"}|200,400|Get collaborative filtering recommendations"
    "GET|/api/recommendation/diverse||200|Get diverse recommendations across categories"

    # --- v6 Additions ---
    "GET|/api/recommendation/by-reading||200|Recommend based on reading history"
    "POST|/api/recommendation/reset|{\"userId\":\"1\"}|200|Reset recommendation model for user"
    "GET|/api/recommendation/explain/1||200|Explain why a paper was recommended"

    # --- Round 21 Additions ---
    "POST|/api/recommendations/train|{\"algorithm\":\"collaborative\"}|200|Trigger model retrain"
    "GET|/api/recommendations/quality||200|Get recommendation quality metrics"
    "POST|/api/recommendations/cross-domain|{\"paperId\":1,\"domains\":[\"ml\",\"nlp\"]}|200|Get cross-domain recommendations"

    # --- Round 25 Additions ---
    "GET|/api/recommendations/engines||200|List recommendation engines"
    "POST|/api/recommendations/explain/1|{}|200|Explain recommendation"
    "POST|/api/recommendations/a-b-test|{\"name\":\"test1\",\"engineA\":\"collaborative\",\"engineB\":\"content\",\"duration\":7}|200|Create A/B test"

    # --- Round 28 Additions ---
    "POST|/api/recommendations/weights|{\"collaborative\":0.4,\"content\":0.3}|200|Set recommendation weights"
    "GET|/api/recommendations/user/1/profile||200|Get user recommendation profile"
    "DELETE|/api/recommendations/cache||200|Clear recommendation cache"

    # --- Round 30 Additions ---
    "POST|/api/recommendations/feedback/batch|{\"feedbacks\":[{\"paperId\":1,\"action\":\"like\"}]}|200|Submit batch feedback"
    "GET|/api/recommendations/trending/topics||200|Get trending topics"

    # --- Round 32 Additions ---
    "POST|/api/recommendations/preferences/reset|{\"userId\":1}|200|Reset preferences"
    "GET|/api/recommendations/papers/1/similar||200|Find similar papers"

    # --- Round 33 Additions ---
    "POST|/api/recommendations/weights|{\"userId\":1,\"weights\":{\"content\":0.5,\"collaborative\":0.3,\"popularity\":0.2}}|200|Set recommendation weights"
    "GET|/api/recommendations/insights?period=week||200|Get recommendation insights"

    # --- Round 34 Additions ---
    "POST|/api/recommendations/schedule|{\"frequency\":\"weekly\",\"categories\":[\"ml\"],\"maxResults\":10}|200|Schedule recommendation refresh"
    "GET|/api/recommendations/trending/categories?period=month||200|Get trending categories"

    # --- Round 35 Additions ---
    "POST|/api/recommendations/blacklist|{\"paperId\":99,\"reason\":\"not_relevant\"}|200|Add to blacklist"
    "GET|/api/recommendations/fresh?limit=5||200|Get fresh recommendations"

    # --- Round 36 Additions ---
    "GET|/api/recommendations/quality/score?userId=1||200|Get recommendation quality score"
    "POST|/api/recommendations/feedback/batch|{\"feedbacks\":[{\"paperId\":1,\"rating\":5,\"action\":\"like\"}]}|200|Submit batch feedback"

    # --- Round 37 Additions ---
    "POST|/api/recommendations/survey|{\"userId\":1,\"interests\":[\"ml\"],\"experienceLevel\":\"intermediate\",\"goals\":[\"research\"]}|200|Submit survey"
    "GET|/api/recommendations/personalized/count?userId=1||200|Get personalized count"

    # --- Round 38 Additions ---
    "PUT|/api/recommendations/preferences/categories|{\"userId\":1,\"categories\":[{\"name\":\"ml\",\"weight\":0.8}]}|200|Update category preferences"
    "GET|/api/recommendations/history/detailed?userId=1&limit=10||200|Get detailed history"

    # --- Round 39 Additions ---
    "POST|/api/recommendations/explain|{\"userId\":1,\"paperIds\":[1,2,3]}|200|Explain recommendations"
    "GET|/api/recommendations/subscription/status?userId=1||200|Get subscription status"

    # --- Round 40 Additions ---
    "POST|/api/recommendations/ab-test/vote|{\"userId\":1,\"testId\":\"rec_v2\",\"preferredSet\":\"B\"}|200|Vote A/B test"
    "GET|/api/recommendations/ab-test/results?testId=rec_v2||200|Get A/B test results"

    # --- Round 41 Additions ---
    "POST|/api/recommendations/collaborative/filter|{\"userId\":1,\"minOverlap\":0.3}|200|Filter collaborative recs"
    "GET|/api/recommendations/diversity/report?userId=1||200|Get diversity report"

    # --- Round 42 Additions ---
    "POST|/api/recommendations/papers/1/alternative|{\"excludeIds\":[2,3],\"minSimilarity\":0.5}|200|Find alternative papers"
    "GET|/api/recommendations/trending/authors?limit=10||200|Get trending authors"

    # --- Round 43 Additions ---
    "POST|/api/recommendations/serendipity|{\"userId\":1,\"diversityFactor\":0.7}|200|Get serendipitous recs"
    "GET|/api/recommendations/user/1/profile||200|Get user rec profile"

    # --- Round 44 Additions ---
    "POST|/api/recommendations/session/start|{\"userId\":1,\"context\":\"browsing\"}|200|Start rec session"
    "GET|/api/recommendations/session/sess_123/events||200|Get session events"

    # --- Round 45 Additions ---
    "POST|/api/recommendations/session/sess_123/track|{\"paperId\":1,\"action\":\"click\",\"dwellTime\":5.2}|200|Track interaction"
    "GET|/api/recommendations/papers/1/related/count||200|Count related papers"

    # --- Round 46 Additions ---
    "POST|/api/recommendations/papers/1/note|{\"note\":\"Interesting\",\"tags\":[\"methodology\"]}|200|Add paper note"
    "GET|/api/recommendations/notes?limit=20||200|Get recommendation notes"

    # --- Round 47 Additions ---
    "PUT|/api/recommendations/notes/1|{\"note\":\"Updated\",\"tags\":[\"updated\"]}|200|Update note"
    "DELETE|/api/recommendations/notes/1||200|Delete note"

    # --- Round 48 Additions ---
    "POST|/api/recommendations/feedback/submit|{\"userId\":\"1\",\"paperId\":1,\"rating\":4,\"comment\":\"Great recommendation\"}|200|Submit feedback on recommendation"
    "GET|/api/recommendations/trending/topics?limit=5||200|Get trending recommendation topics"

    # --- Round 49 Additions ---
    "POST|/api/recommendations/sessions/create|{\"userId\":\"1\",\"preferences\":{\"categories\":[\"ml\"]}}|200|Create recommendation session"
    "GET|/api/recommendations/sessions/rs_123/status||200|Get session status"

    # --- Round 50 Additions ---
    "POST|/api/recommendations/blacklist/add|{\"userId\":\"1\",\"paperId\":99,\"reason\":\"not_relevant\"}|200|Add paper to recommendation blacklist"
    "GET|/api/recommendations/blacklist?userId=1||200|Get user's blacklist"

    # --- Round 51 Additions ---
    "POST|/api/recommendations/preferences/update|{\"userId\":\"1\",\"weights\":{\"content\":0.5,\"collaborative\":0.3},\"categories\":[\"ml\",\"nlp\"]}|200|Update recommendation preferences"
    "GET|/api/recommendations/papers/similar/1?limit=5||200|Find similar papers with similarity scores"

    # --- Round 52 Additions ---
    "POST|/api/recommendations/collections/create|{\"name\":\"My Collection\",\"description\":\"Test\",\"paperIds\":[1,2,3]}|200|Create a paper collection"
    "GET|/api/recommendations/collections/col_123||200|Get collection details"

    # --- Round 53 Additions ---
    "POST|/api/recommendations/explanations/request|{\"userId\":\"1\",\"paperId\":\"42\"}|200|Request explanation for a recommendation"
    "GET|/api/recommendations/papers/trending?period=week&limit=10||200|Get trending papers with growth metrics"

    # --- Round 54 Additions ---
    "POST|/api/recommendations/preferences/export|{\"userId\":\"1\"}|200|Export user recommendation preferences"
    "GET|/api/recommendations/papers/discover?limit=5&algorithm=hybrid||200|Discover papers via recommendation algorithms"

    # --- Round 55 Additions ---
    "POST|/api/recommendations/learning-path|{\"userId\":\"1\",\"maxSteps\":5}|200|Generate a learning path recommendation"
    "GET|/api/recommendations/sentiment/summary?userId=1&period=month||200|Get recommendation sentiment summary"

    # --- Round 56 Additions ---
    "POST|/api/recommendations/relevance/tune|{\"contentWeight\":0.4,\"collaborativeWeight\":0.3,\"freshnessWeight\":0.2,\"popularityWeight\":0.1}|200|Tune recommendation relevance parameters"
    "GET|/api/recommendations/collections?userId=1&limit=10||200|List all recommendation collections"

    # --- Round 57 Additions ---
    "POST|/api/recommendations/relevance/feedback|{\"userId\":\"1\",\"paperId\":42,\"relevanceScore\":0.85,\"comment\":\"Highly relevant\"}|200|Submit relevance feedback for fine-tuning"
    "GET|/api/recommendations/cluster-analysis?userId=1&limit=10&algorithm=kmeans||200|Get paper cluster analysis recommendations"

    # --- Round 58 Additions ---
    "POST|/api/recommendations/neighborhood-graph|{\"userId\":\"1\",\"maxDepth\":2,\"minSimilarity\":0.5}|200|Build a recommendation neighborhood graph"
    "GET|/api/recommendations/seasonal-trends?period=current&limit=5||200|Get seasonal recommendation trends"

    # --- Round 59 Additions ---
    "POST|/api/recommendations/influence-map|{\"userId\":\"1\",\"maxNodes\":20,\"minInfluence\":0.3}|200|Build a recommendation influence map"
    "GET|/api/recommendations/novelty-score?userId=1&limit=10&algorithm=hybrid||200|Get novelty score for recommendations"

    # --- Round 60 Additions ---
    "POST|/api/recommendations/recency-decay|{\"userId\":\"1\",\"decayFactor\":0.9,\"halfLifeDays\":30}|200|Apply recency decay to recommendations"
    "GET|/api/recommendations/topology-map?userId=1&depth=3&limit=15||200|Get recommendation topology map"

    # --- Round 61 Additions ---
    "POST|/api/recommendations/semantic-cluster|{\"userId\":\"1\",\"maxClusters\":5,\"minCohesion\":0.6}|200|Cluster papers by semantic similarity"
    "GET|/api/recommendations/evolution-timeline?userId=1&period=month&steps=6||200|Get recommendation evolution over time"

    # --- Round 62 Additions ---
    "POST|/api/recommendations/attention-weight|{\"userId\":\"1\",\"attentionScale\":1.2,\"decayRate\":0.95,\"topK\":10}|200|Adjust attention-based recommendation weights"
    "GET|/api/recommendations/drift-detection?userId=1&windowSize=7d&sensitivity=0.5&limit=10||200|Detect concept drift in recommendation patterns"

    # --- Round 63 Additions ---
    "POST|/api/recommendations/reinforcement-signal|{\"userId\":\"1\",\"paperId\":\"42\",\"action\":\"click\",\"reward\":0.85}|200|Submit reinforcement learning signal"
    "GET|/api/recommendations/embedding-projection?userId=1&method=umap&limit=10||200|Get 2D projection of paper embeddings"

    # --- Round 64 Additions ---
    "POST|/api/recommendations/temporal-preference|{\"userId\":\"1\",\"morningWeight\":0.25,\"afternoonWeight\":0.35,\"eveningWeight\":0.25,\"nightWeight\":0.15}|200|Set temporal preference weights"
    "GET|/api/recommendations/citation-network?userId=1&depth=2&limit=10&minCitationStrength=0.3||200|Get citation network graph"

    # --- Round 65 Additions ---
    "POST|/api/recommendations/contextual-rank|{\"userId\":\"1\",\"context\":\"research\",\"topK\":10,\"diversityBoost\":0.1}|200|Contextual ranking of recommendations"
    "GET|/api/recommendations/preference-evolution?userId=1&period=month&steps=6||200|Track preference evolution over time"

    # --- Round 66 Additions ---
    "POST|/api/recommendations/multi-objective|{\"userId\":\"1\",\"topK\":10,\"accuracyWeight\":0.4,\"diversityWeight\":0.3,\"noveltyWeight\":0.2,\"recencyWeight\":0.1}|200|Multi-objective recommendation optimization"
    "GET|/api/recommendations/fairness-audit?userId=1&limit=10&threshold=0.5||200|Audit recommendation fairness across categories"

    # --- Round 67 Additions ---
    "POST|/api/recommendations/knowledge-transfer|{\"userId\":\"1\",\"sourceDomain\":\"ml\",\"targetDomain\":\"nlp\",\"topK\":10,\"transferWeight\":0.5}|200|Transfer learning-based knowledge recommendations"
    "GET|/api/recommendations/graph-embedding?userId=1&method=node2vec&dimensions=64&limit=10||200|Get graph embedding visualization for recommendations"

    # --- Round 68 Additions ---
    "POST|/api/recommendations/bandit-feedback|{\"userId\":\"1\",\"paperId\":42,\"reward\":0.85,\"armContext\":\"collaborative\"}|200|Submit bandit algorithm feedback for recommendation tuning"
    "GET|/api/recommendations/exploration-map?userId=1&limit=10&coverageThreshold=0.3||200|Get exploration coverage map for recommendation domains"

    # --- Round 69 Additions ---
    "POST|/api/recommendations/preference-diffusion|{\"userId\":\"1\",\"diffusionRate\":0.5,\"iterations\":10,\"convergenceThreshold\":0.01}|200|Diffuse user preferences across categories"
    "GET|/api/recommendations/recall-precision?userId=1&limit=10&minPrecision=0.0||200|Get recall and precision metrics for recommendation engines"

    # --- Round 70 Additions ---
    "POST|/api/recommendations/interest-graph|{\"userId\":\"1\",\"maxNodes\":20,\"minInterest\":0.3}|200|Build interest graph for recommendations"
    "GET|/api/recommendations/diversity-index?userId=1&limit=10&threshold=0.5||200|Get diversity index metrics for recommendations"

    # --- Round 71 Additions ---
    "POST|/api/recommendations/preference-conflict|{\"userId\":\"1\",\"conflictThreshold\":0.5}|200|Detect preference conflicts"
    "GET|/api/recommendations/engagement-heatmap?userId=1&gridSize=10||200|Get engagement heatmap"

    # --- Round 72 Additions ---
    "POST|/api/recommendations/preference-cascade|{\"userId\":\"1\",\"maxDepth\":3,\"cascadeStrength\":0.5}|200|Cascade preference influence"
    "GET|/api/recommendations/recommendation-momentum?userId=1&windowDays=30||200|Get recommendation momentum"

    # --- Round 73 Additions ---
    "POST|/api/recommendations/preference-synthesis|{\"userId\":\"1\",\"maxSignals\":10,\"confidenceThreshold\":0.5}|200|Synthesize preferences from multiple signals"
    "GET|/api/recommendations/knowledge-gap?userId=1&depth=3&domain=ml||200|Identify knowledge gaps in user reading"

    # --- Round 74 Additions ---
    "POST|/api/recommendations/cross-domain-bridge|{\"userId\":\"1\",\"sourceDomain\":\"ml\",\"targetDomain\":\"nlp\",\"topK\":10,\"bridgeStrength\":0.5}|200|Bridge recommendations across domains"
    "GET|/api/recommendations/preference-stability?userId=1&windowDays=30&threshold=0.5||200|Measure preference stability over time"

    # --- Round 75 Additions ---
    "POST|/api/recommendations/domain-fusion|{\"userId\":\"1\",\"domains\":[\"ml\",\"nlp\"],\"topK\":10,\"fusionWeight\":0.5}|200|Fuse recommendations across multiple domains"
    "GET|/api/recommendations/preference-anchor?userId=1&limit=10&minConfidence=0.3||200|Get preference anchor points"

    # --- Round 76 Additions ---
    "POST|/api/recommendations/preference-reconcile|{\"userId\":\"1\",\"conflictThreshold\":0.5,\"maxIterations\":10}|200|Reconcile conflicting preference signals"
    "GET|/api/recommendations/trending-spectrum?period=week&limit=10&minGrowth=0.0||200|Get trending topic spectrum analysis"

    # --- Round 77 Additions ---
    "POST|/api/recommendations/preference-harmonize|{\"userId\":\"1\",\"harmonizationStrength\":0.5,\"maxRounds\":5}|200|Harmonize preferences across overlapping domains"
    "GET|/api/recommendations/interest-volatility?userId=1&windowDays=30&granularity=7||200|Get interest volatility metrics"

    # --- Round 78 Additions ---
    "POST|/api/recommendations/attention-budget|{\"userId\":\"1\",\"explorationRatio\":0.3,\"totalBudget\":20}|200|Allocate attention budget across domains"
    "GET|/api/recommendations/recommendation-lifecycle?userId=1&limit=50||200|Track recommendation lifecycle stages"

    # --- Round 79 Additions ---
    "POST|/api/recommendations/taste-profile|{\"userId\":\"1\",\"maxDimensions\":10,\"confidenceThreshold\":0.3}|200|Build user reading taste profile"
    "GET|/api/recommendations/citation-velocity?userId=1&limit=20&period=month||200|Track citation velocity of recommended papers"

    # --- Round 80 Additions ---
    "POST|/api/recommendations/taste-vector|{\"userId\":\"1\",\"maxDimensions\":8,\"confidenceThreshold\":0.3}|200|Compute multi-dimensional taste vector"
    "GET|/api/recommendations/reading-radar?userId=1&limit=12&depth=3||200|Generate reading radar profile across domains"

    # --- Round 81 Additions ---
    "POST|/api/recommendations/reading-syllabus|{\"userId\":\"1\",\"topic\":\"machine_learning\",\"maxWeeks\":8,\"difficulty\":\"intermediate\",\"pace\":\"moderate\"}|200|Generate curated reading syllabus with progressive difficulty"
    "GET|/api/recommendations/serendipity-discover?userId=1&limit=10&minSurprise=0.3&bridgeStrategy=structural_hole||200|Discover serendipitous papers bridging unexpected domain connections"

    # --- Round 82 Additions ---
    "POST|/api/recommendations/reading-velocity|{\"userId\":\"1\",\"windowDays\":30,\"projectionMonths\":6}|200|Compute reading velocity profile with domain projections"
    "GET|/api/recommendations/knowledge-frontier?userId=1&limit=10&minBridgeScore=0.3&depth=adjacent||200|Identify zone of proximal development research papers"

    # --- Round 83 Additions ---
    "POST|/api/recommendations/influence-cascade|{\"userId\":\"1\",\"decayFactor\":0.85,\"maxHops\":3,\"minInfluence\":0.1,\"topK\":10}|200|Propagate influence scores through citation network"
    "GET|/api/recommendations/reading-constellation?userId=1&limit=10&bridgeThreshold=0.4&minClusterSize=2||200|Generate reading constellation map with thematic clusters"

    # --- Round 84 Additions ---
    "POST|/api/recommendations/reading-resonance|{\"userId\":\"1\",\"resonanceThreshold\":0.3,\"limit\":10,\"period\":\"month\"}|200|Compute reading resonance with emerging trends"
    "GET|/api/recommendations/bibliographic-coupling?userId=1&minSharedRefs=2&limit=10&minCouplingStrength=0.1||200|Find papers via bibliographic coupling analysis"

    # --- Round 85 Additions ---
    "POST|/api/recommendations/co-reading-network|{\"userId\":\"1\",\"minOverlap\":0.2,\"maxNeighbors\":10,\"topK\":10}|200|Build co-reading network for collaborative discovery"
    "GET|/api/recommendations/impact-trail?userId=1&maxDepth=3&limit=10&minImpact=0.1||200|Trace citation impact trail from reading history"

    # --- Round 86 Additions ---
    "POST|/api/recommendations/reading-whisperer|{\"userId\":\"1\",\"weakSignalThreshold\":0.15,\"limit\":8,\"bridgeStrategy\":\"structural_hole\"}|200|Suggest niche papers via weak-signal detection"
    "GET|/api/recommendations/citation-constellation?userId=1&depth=2&limit=15&minClusterCohesion=0.3||200|Map citation constellation with bridge stars"

    # --- Round 87 Additions ---
    "POST|/api/recommendations/reading-companion|{\"userId\":\"1\",\"mode\":\"bridge\",\"topK\":10,\"minRelevance\":0.3}|200|Suggest companion papers bridging topical gaps"
    "GET|/api/recommendations/influence-ripple?userId=1&maxDepth=3&limit=20&decayFactor=0.7&sortBy=influence||200|Track influence ripple through citation graph"

    # --- Round 88 Additions ---
    "POST|/api/recommendations/reading-momentum|{\"userId\":\"1\",\"windowDays\":30,\"topK\":10,\"momentumBoost\":0.2}|200|Compute reading momentum with acceleration trajectory"
    "GET|/api/recommendations/research-horizon?userId=1&depth=2&limit=10&minBridgeScore=0.3&strategy=adjacent||200|Scan research horizon at knowledge boundary"

    # --- Round 89 Additions ---
    "POST|/api/recommendations/reading-spark|{\"userId\":\"1\",\"windowDays\":90,\"topK\":10,\"shiftThreshold\":0.25}|200|Identify spark papers igniting new research interests"
    "GET|/api/recommendations/braintrust?userId=1&limit=10&minInfluence=0.3||200|Curate brain trust of influential domain authors"

    # --- Round 90 Additions ---
    "POST|/api/recommendations/reading-symbiosis|{\"userId\":\"1\",\"topK\":10,\"minSynergy\":0.3,\"pairingStrategy\":\"complementary\"}|200|Identify symbiotic reading pairs with synergy scores"
    "GET|/api/recommendations/orphan-gems?userId=1&limit=10&minRelevance=0.3&maxCitations=50||200|Discover high-quality under-cited orphan gem papers"

    # --- Round 91 Additions ---
    "POST|/api/recommendations/reading-ancestry|{\"userId\":\"1\",\"maxDepth\":3,\"topK\":10,\"minInfluence\":0.1}|200|Trace intellectual ancestry of reading history"
    "GET|/api/recommendations/cognitive-load?userId=1&limit=10&targetLevel=intermediate||200|Get cognitive load profile for difficulty-appropriate recommendations"

    # --- Round 92 Additions ---
    "POST|/api/recommendations/reading-trajectory|{\"userId\":\"usr_123\",\"lookbackDays\":90,\"projectionDays\":30,\"granularity\":\"weekly\"}|200|Analyze reading trajectory"
    "GET|/api/recommendations/knowledge-frontier?userId=usr_123&field=computer+science&depth=2&limit=10||200|Get knowledge frontier analysis"

    # --- Round 93 Additions ---
    "POST|/api/recommendations/serendipity-engine|{\"userId\":\"usr_123\",\"serendipityLevel\":0.7,\"domain\":\"computer science\",\"limit\":10}|200|Get serendipitous paper discoveries"
    "GET|/api/recommendations/reading-velocity?userId=usr_123&period=30d&granularity=daily||200|Get reading velocity analysis"

    # --- Round 94 Additions ---
    "POST|/api/recommendations/influence-tracker|{\"userId\":\"usr_123\",\"paperIds\":[\"p1\",\"p2\",\"p3\"],\"metrics\":[\"citations\",\"social_mentions\"]}|200|Track paper influence"
    "GET|/api/recommendations/discovery-timeline?userId=usr_123&months=6||200|Get discovery timeline"

    # --- Round 95 Additions ---
    "POST|/api/recommendations/citation-network/personal|{\"userId\":\"usr_123\",\"depth\":2,\"minCitations\":5,\"timeRange\":\"5y\"}|200|Build personal citation network"
    "GET|/api/recommendations/reading-comfort?userId=usr_123&sessions=20||200|Analyze reading comfort"

    # --- Round 96 Additions ---
    "POST|/api/recommendations/reading-mood|{\"userId\":\"usr_123\",\"mood\":\"curious\",\"availableTime\":30,\"energyLevel\":\"high\"}|200|Get mood-based recommendations"
    "GET|/api/recommendations/field-evolution?field=NLP&years=5||200|Get field evolution analysis"

    # --- Round 97 Additions ---
    "GET|/api/recommendations/seasonal-papers?season=spring&year=2026||200|Get seasonal paper recommendations"
    "POST|/api/recommendations/preference/reset|{\"userId\":\"1\",\"resetCategories\":true,\"resetWeights\":true,\"resetHistory\":false}|200|Reset recommendation preferences"

    # --- Round 98 Additions ---
    "GET|/api/recommendations/cross-domain?sourceDomain=ml&targetDomain=nlp||200|Get cross-domain paper recommendations"
    "POST|/api/recommendations/feedback/batch|{\"items\":[{\"paperId\":1,\"action\":\"like\",\"rating\":5},{\"paperId\":2,\"action\":\"skip\"}]}|200|Submit batch feedback on recommendations"

    # --- Round 99 Additions ---
    "GET|/api/recommendation/diversity-score?userId=1||200|Get recommendation diversity score"
    "POST|/api/recommendation/blacklist/add|{\"paperId\":42,\"reason\":\"not_relevant\"}|200|Add paper to recommendation blacklist"

    # --- Round 100 Additions ---
    "GET|/api/recommendation/trending-authors?field=ml&limit=5||200|Get trending authors"
    "POST|/api/recommendation/serendipity/trigger|{\"userId\":\"1\",\"interests\":[\"ml\",\"nlp\"]}|200|Trigger serendipitous discovery"

    # --- Round 101 Additions ---
    "GET|/api/recommendation/reading-time/estimate?paperIds=p1,p2,p3||200|Estimate reading time for papers"
    "POST|/api/recommendation/interest/update|{\"userId\":\"1\",\"interests\":[\"ml\",\"nlp\"],\"weights\":{\"content\":0.5,\"collaborative\":0.3}}|200|Update user interest profile"

    # --- Round 102 Additions ---
    "GET|/api/recommendations/collaborative-papers||200|Get papers frequently co-cited together"
    "GET|/api/recommendations/collaborative-papers?paperId=p_101||200|Get co-cited papers filtered by paper ID"
    "POST|/api/recommendations/exploration/start|{\"topic\":\"machine_learning\",\"depth\":3}|200|Start a topic exploration session"
    "POST|/api/recommendations/exploration/start|{}|200|Start exploration session with defaults"

    # --- Round 103 Additions ---
    "GET|/api/recommendation/paper/network?paperId=p1&depth=2||200|Get paper citation network"
    "POST|/api/recommendation/preference/import|{\"source\":\"mendeley\",\"preferences\":[{\"userId\":\"1\",\"category\":\"ml\",\"weight\":0.8}]}|200|Import user preferences from external source"

    # --- Round 104 Additions ---
    "GET|/api/recommendation/similarity/matrix?paperIds=p1,p2,p3||200|Get paper similarity matrix"
    "POST|/api/recommendation/weight/adjust|{\"category\":\"ml\",\"weight\":0.8}|200|Adjust recommendation weights"

    # --- Round 105 Additions ---
    "GET|/api/recommendation/map/visualize?centerPaperId=p1&radius=3||200|Visualize recommendation map"
    "POST|/api/recommendation/history/clear|{\"userId\":\"1\",\"olderThanDays\":30}|200|Clear recommendation history"

    # --- Round 106 Additions ---
    "GET|/api/recommendations/related-fields?field=machine_learning||200|Get related research fields"
    "POST|/api/recommendations/feedback/export|{\"userId\":\"1\",\"format\":\"json\"}|200|Export user feedback data"

    # --- Round 107 Additions ---
    "GET|/api/recommendations/author/recommend?field=ml&limit=5||200|Recommend authors to follow"
    "POST|/api/recommendations/session/start|{\"userId\":\"1\",\"context\":\"browsing\"}|200|Start a recommendation session"

    # --- Route 196-197 Additions ---
    "GET|/api/recommendations/topic/trending?period=week&limit=5||200|Get trending research topics with growth rates"
    "POST|/api/recommendations/preference/batch|{\"preferences\":[{\"category\":\"ml\",\"weight\":0.8},{\"category\":\"nlp\",\"weight\":0.6}]}|200|Batch update preference weights"

    # --- Route 198-199 Additions ---
    "GET|/api/recommendations/insight/daily?userId=1||200|Get daily research insight"
    "POST|/api/recommendations/comparison/save|{\"paperIds\":[1,2,3],\"notes\":\"Comparing transformer architectures\"}|200|Save a paper comparison"

    # --- Route 200-201 Additions ---
    "GET|/api/recommendation/conference/match?paperId=1&limit=5||200|Match papers to suitable conferences"
    "POST|/api/recommendation/survey/response|{\"responses\":[{\"questionId\":\"q1\",\"answer\":\"satisfied\"},{\"questionId\":\"q2\",\"answer\":\"very relevant\"}],\"overallRating\":4.5}|200|Submit survey response about recommendations"
)
