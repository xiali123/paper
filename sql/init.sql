-- PaperCrawler Database Schema

-- Create database
CREATE DATABASE IF NOT EXISTS csdatabs
CHARACTER SET utf8mb4
COLLATE utf8mb4_unicode_ci;

USE csdatabs;

-- Papers table
CREATE TABLE IF NOT EXISTS cspaper (
    id INT AUTO_INCREMENT PRIMARY KEY,
    kid INT NOT NULL DEFAULT 0,
    type VARCHAR(50),
    title TEXT,
    qikanfull VARCHAR(255),
    qikanjc VARCHAR(100),
    year VARCHAR(10),
    author VARCHAR(255),
    qikanurl VARCHAR(512),
    doiurl VARCHAR(512),
    info TEXT,
    qkid INT DEFAULT 0,
    level VARCHAR(10),
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    INDEX idx_type (type),
    INDEX idx_year (year),
    INDEX idx_qkid (qkid),
    INDEX idx_level (level)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- Journals table
CREATE TABLE IF NOT EXISTS qikantb (
    id INT AUTO_INCREMENT PRIMARY KEY,
    name VARCHAR(100) UNIQUE,
    fullname VARCHAR(255),
    level VARCHAR(10),
    flevel VARCHAR(10),
    info TEXT,
    url VARCHAR(512),
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    INDEX idx_name (name),
    INDEX idx_level (level)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- Insert sample data (optional)
-- INSERT INTO qikantb (name, fullname, level, flevel) VALUES
-- ('CVPR', 'Conference on Computer Vision and Pattern Recognition', 'A', 'A'),
-- ('ICCV', 'International Conference on Computer Vision', 'A', 'A');
