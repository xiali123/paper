-- ============================================================================
-- PaperCrawler Test Data
-- Description: Sample papers, journals, authors for testing
-- ============================================================================

-- ============================================================================
-- Papers
-- ============================================================================

INSERT INTO papers (title, authors, year, publication, volume, issue, pages, doi, abstract, keywords, citation_count, is_favorite, tags) VALUES
(
    'Attention Is All You Need',
    '["Ashish Vaswani", "Noam Shazeer", "Niki Parmar", "Jakob Uszkoreit", "Llion Jones", "Aidan N. Gomez", "Łukasz Kaiser", "Illia Polosukhin"]',
    2017,
    'NeurIPS',
    '30',
    NULL,
    '599-611',
    '10.5554/abs/1706.03762',
    'The dominant sequence transduction models are based on complex recurrent or convolutional neural networks that include an encoder and a decoder. The best performing models also connect the encoder and decoder through an attention mechanism. We propose a new simple network architecture, the Transformer, based solely on attention mechanisms, dispensing with recurrence and convolutions entirely.',
    '["attention", "transformer", "neural networks", "NLP", "sequence transduction"]',
    50000,
    1,
    '["deep learning", "NLP", "must-read"]'
),
(
    'BERT: Pre-training of Deep Bidirectional Transformers for Language Understanding',
    '["Jacob Devlin", "Ming-Wei Chang", "Kenton Lee", "Kristina Toutanova"]',
    2018,
    'NAACL',
    '1',
    NULL,
    '4171-4186',
    '10.18653/v1/P18-1098',
    'We introduce a new language representation model called BERT, which stands for Bidirectional Encoder Representations from Transformers. Unlike recent language representation models, BERT is designed to pre-train deep bidirectional representations from unlabeled text by jointly conditioning on both left and right context in all layers.',
    '["BERT", "pre-training", "language model", "NLP", "transformer"]',
    80000,
    1,
    '["deep learning", "NLP", "transformer", "state-of-the-art"]'
),
(
    'ResNet: Deep Residual Learning for Image Recognition',
    '["Kaiming He", "Xiangyu Zhang", "Shaoqing Ren", "Jian Sun"]',
    2015,
    'CVPR',
    NULL,
    NULL,
    '770-778',
    '10.1109/CVPR.2016.90',
    'Deeper neural networks are more difficult to train. We present a residual learning framework to ease the training of networks that are substantially deeper than those used previously. We explicitly reformulate the layers as learning residual functions with reference to the layer inputs, instead of learning unreferenced functions.',
    '["ResNet", "deep learning", "computer vision", "CNN", "image recognition"]',
    120000,
    1,
    '["deep learning", "computer vision", "CNN", "classic"]'
),
(
    'GPT-4 Technical Report',
    '["OpenAI"]',
    2023,
    'arXiv',
    NULL,
    NULL,
    NULL,
    '10.48550/arXiv.2303.08774',
    'We report the development of GPT-4, a large-scale, multimodal model which can accept image and text inputs and produce text outputs. While GPT-4 is less capable than humans in many real-world scenarios, it exhibits human-level performance on various professional and academic benchmarks.',
    '["GPT-4", "large language model", "multimodal", "AI", "transformer"]',
    10000,
    1,
    '["AI", "LLM", "state-of-the-art", "must-read"]'
),
(
    'Diffusion Models Beat GANs on Image Synthesis',
    '["Prafulla Dhariwal", "Alexander Nichol"]',
    2021,
    'NeurIPS',
    '34',
    NULL,
    '21178-21196',
    '10.5554/abs/2105.05233',
    'We show that diffusion models can achieve image sample quality superior to the current state-of-the-art generative models. We achieve this in part by designing a new model architecture and by training on a large dataset of images.',
    '["diffusion models", "GANs", "image synthesis", "generative models"]',
    3000,
    0,
    '["deep learning", "generative AI", "computer vision"]'
),
(
    'YOLOv7: Trainable Bag-of-Freebies Sets New State-of-the-Art for Real-Time Object Detectors',
    '["Chien-Yao Wang", "Alexey Bochkovskiy", "Hung-Yu Tseng"]',
    2023,
    'arXiv',
    NULL,
    NULL,
    NULL,
    '10.48550/arXiv.2207.02696',
    'YOLOv7 surpasses all known object detectors in both speed and accuracy in the range from 5 FPS to 160 FPS and has the highest accuracy (56.8% AP) among all real-time object detectors.',
    '["YOLO", "object detection", "computer vision", "real-time"]',
    2000,
    1,
    '["computer vision", "object detection", "YOLO"]'
),
(
    'Language Models are Few-Shot Learners',
    '["Tom B. Brown", "Benjamin Mann", "Nick Ryder", "Melanie Subbiah", "Jared Kaplan", "Prafulla Dhariwal", "Arvind Neelakantan", "Pranav Shyam", "Girish Sastry", "Askith Vallur"]',
    2020,
    'NeurIPS',
    '33',
    NULL,
    '1877-1901',
    '10.48550/arXiv.2005.14165',
    'Recent work has demonstrated substantial gains on many NLP tasks and benchmarks by pre-training on a large corpus of text followed by fine-tuning on a specific task. We demonstrate that scaling up language models greatly improves task-agnostic, few-shot performance, sometimes even reaching competitiveness with prior state-of-the-art fine-tuning approaches.',
    '["GPT-3", "few-shot learning", "language models", "NLP"]',
    15000,
    0,
    '["NLP", "few-shot", "GPT", "LLM"]'
),
(
    'Denoising Diffusion Probabilistic Models',
    '["Jonathan Ho", "Ajay Jain", "Pieter Abbeel"]',
    2020,
    'NeurIPS',
    '33',
    NULL,
    '6840-6850',
    '10.48550/arXiv.2006.11239',
    'We present high quality image synthesis results using diffusion probabilistic models, a class of latent variable models inspired by considerations from nonequilibrium thermodynamics.',
    '["diffusion models", "generative models", "image synthesis"]',
    8000,
    0,
    '["deep learning", "generative AI", "diffusion"]'
),
(
    'EfficientNet: Rethinking Model Scaling for Convolutional Neural Networks',
    '["Mingxing Tan", "Quoc V. Le"]',
    2019,
    'ICML',
    NULL,
    NULL,
    '4809-4816',
    '10.48550/arXiv.1905.11946',
    'Convolutional Neural Networks (ConvNets) are commonly developed at a fixed resource budget, and then scaled up in order to achieve higher accuracy. In this paper, we systematically study model scaling and identify that carefully balancing network depth, width, and resolution can lead to better performance.',
    '["EfficientNet", "CNN", "model scaling", "computer vision"]',
    9000,
    0,
    '["deep learning", "CNN", "efficiency"]'
),
(
    'LoRA: Low-Rank Adaptation of Large Language Models',
    '["Edward J. Hu", "Yelong Shen", "Phillip Wallis", "Zeyuan Allen", "Yi Li", "Seyoung Wang", "Lu Wang", "Weizhu Chen"]',
    2021,
    'arXiv',
    NULL,
    NULL,
    NULL,
    '10.48550/arXiv.2106.09685',
    'LoRA is an efficient fine-tuning approach that freezes pretrained model weights and injects trainable rank decomposition matrices into each layer of the Transformer architecture, greatly reducing the number of trainable parameters for downstream tasks.',
    '["LoRA", "fine-tuning", "LLM", "efficient training"]',
    5000,
    1,
    '["LLM", "fine-tuning", "practical", "must-read"]'
);

-- ============================================================================
-- Journals
-- ============================================================================

INSERT INTO journals (name, publisher, issn, tier, impact_factor) VALUES
('NeurIPS', 'Neural Information Processing Systems Foundation', '2334-0034', 'Tier 1', 8.2),
('NAACL', 'Association for Computational Linguistics', '2372-3366', 'Tier 1', 7.5),
('CVPR', 'IEEE Computer Society', '2374-8216', 'Tier 1', 9.1),
('ICML', 'International Machine Learning Society', '2374-8218', 'Tier 1', 8.8),
('arXiv', 'Cornell University', NULL, 'Tier 1', NULL),
('Nature', 'Nature Publishing Group', '0028-0836', 'Tier 1', 49.9),
('Science', 'AAAS', '0036-8075', 'Tier 1', 50.5),
('Cell', 'Cell Press', '0092-8674', 'Tier 1', 41.5),
('JMLR', 'Microtome Publishing', '1532-4438', 'Tier 2', 4.2),
('PAMI', 'IEEE', '0162-8828', 'Tier 1', 9.3);

-- ============================================================================
-- Authors
-- ============================================================================

INSERT INTO authors (name, affiliation, orcid, paper_count, citation_count, h_index) VALUES
('Ashish Vaswani', 'Google Brain', '0000-0002-6464-0538', 5, 50000, 60),
('Jacob Devlin', 'Google Research', '0000-0002-7786-2594', 3, 80000, 45),
('Kaiming He', 'FAIR Research', '0000-0002-7806-3263', 8, 200000, 85),
('Geoffrey Hinton', 'University of Toronto', '0000-0002-6082-4279', 25, 150000, 120),
('Yann LeCun', 'New York University', '0000-0002-6171-5797', 20, 120000, 100),
('Yoshua Bengio', 'University of Montreal', '0000-0002-4048-7560', 18, 100000, 95),
('OpenAI', 'OpenAI', NULL, 10, 50000, 70),
('Tom B. Brown', 'OpenAI', '0000-0002-7899-4943', 6, 30000, 40),
('Pieter Abbeel', 'UC Berkeley', '0000-0002-7074-3921', 12, 60000, 65),
('Quoc V. Le', 'Google Research', '0000-0002-6647-8655', 7, 40000, 55);

-- ============================================================================
-- Collections
-- ============================================================================

INSERT INTO collections (name, description, is_public, paper_count) VALUES
('Must-Read Papers', 'Important papers that everyone should read', 1, 0),
('Deep Learning Classics', 'Foundational deep learning papers', 1, 0),
('LLM Research', 'Large Language Model papers', 1, 0),
('Computer Vision', 'Computer vision and image processing papers', 1, 0),
('NLP Essentials', 'Natural Language Processing must-reads', 1, 0);

-- ============================================================================
-- Collection Papers (add some papers to collections)
-- ============================================================================

-- Add papers to "Must-Read Papers" collection (collection_id = 1)
INSERT INTO collection_papers (collection_id, paper_id, notes) VALUES
(1, 1, 'Revolutionary attention mechanism'),
(1, 2, 'Foundation of modern NLP'),
(1, 3, 'Classic ResNet paper'),
(1, 4, 'State-of-the-art LLM');

-- Add papers to "Deep Learning Classics" collection (collection_id = 2)
INSERT INTO collection_papers (collection_id, paper_id, notes) VALUES
(2, 1, 'Attention paper'),
(2, 2, 'BERT paper'),
(2, 3, 'ResNet paper'),
(2, 9, 'EfficientNet paper');

-- Add papers to "LLM Research" collection (collection_id = 3)
INSERT INTO collection_papers (collection_id, paper_id, notes) VALUES
(3, 2, 'BERT pre-training'),
(3, 4, 'GPT-4'),
(3, 7, 'GPT-3 few-shot learning'),
(3, 10, 'LoRA fine-tuning');

-- ============================================================================
-- Paper-Authors Relationships
-- ============================================================================

-- Attention paper authors
INSERT INTO paper_authors (paper_id, author_id, author_order, is_corresponding) VALUES
(1, 1, 1, 0),
(1, 2, 2, 0),
(1, 3, 3, 0),
(1, 4, 4, 0),
(1, 5, 5, 0),
(1, 6, 6, 0),
(1, 7, 7, 0),
(1, 8, 8, 0);

-- BERT paper authors
INSERT INTO paper_authors (paper_id, author_id, author_order, is_corresponding) VALUES
(2, 2, 1, 0),
(2, 11, 2, 0),
(2, 12, 3, 0),
(2, 13, 4, 0);

-- ResNet paper authors
INSERT INTO paper_authors (paper_id, author_id, author_order, is_corresponding) VALUES
(3, 3, 1, 0),
(3, 14, 2, 0),
(3, 15, 3, 0),
(3, 16, 4, 0);

-- GPT-4 paper
INSERT INTO paper_authors (paper_id, author_id, author_order, is_corresponding) VALUES
(4, 7, 1, 0);

-- EfficientNet paper
INSERT INTO paper_authors (paper_id, author_id, author_order, is_corresponding) VALUES
(9, 14, 1, 0),
(9, 16, 2, 0);

-- ============================================================================
-- Search History (simulated searches)
-- ============================================================================

INSERT INTO search_history (query, filters, result_count, search_duration_ms) VALUES
('attention mechanism', '{"keywords": ["attention", "transformer"]}', 15, 45),
('BERT language model', '{"year_from": 2018, "year_to": 2019}', 8, 32),
('residual learning', '{"keywords": ["resnet", "residual"]}', 12, 28),
('GPT-4', '{"keywords": ["gpt", "llm"]}', 6, 19);

-- ============================================================================
-- Update Statistics
-- ============================================================================

-- Update journal paper counts
UPDATE journals SET paper_count = (
    SELECT COUNT(*) FROM papers WHERE publication = journals.name
);

-- Note: This would need a more complex query or trigger in production
