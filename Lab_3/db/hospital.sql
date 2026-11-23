-- =============================================================================
--  实验三：QtSQL 数据库模块与 View-Model 结构的使用
--  医院诊疗系统 —— 数据库初始化脚本（MySQL 8.0）
--
--  用法（任选其一）：
--    1) MySQL Workbench：用 root 打开本文件，点击执行（闪电按钮）
--    2) 命令行：mysql -uroot -p < hospital.sql
--
--  说明：脚本可重复执行（建库/建表/种子数据都是 IF NOT EXISTS / INSERT IGNORE），
--        不会清空已有数据。如需彻底重建，先执行：
--        DROP DATABASE IF EXISTS hospital;
-- =============================================================================

SET NAMES utf8mb4;
SET FOREIGN_KEY_CHECKS = 1;

CREATE DATABASE IF NOT EXISTS hospital
  DEFAULT CHARACTER SET utf8mb4
  COLLATE utf8mb4_0900_ai_ci;

USE hospital;

-- -----------------------------------------------------------------------------
-- 1. 用户表（登录 / 注册）
-- -----------------------------------------------------------------------------
CREATE TABLE IF NOT EXISTS `User` (
  ID        VARCHAR(32) NOT NULL COMMENT '用户ID（UUID）',
  FULLNAME  VARCHAR(20) NOT NULL COMMENT '姓名',
  USERNAME  VARCHAR(20) NOT NULL COMMENT '登录名',
  PASSWORD  CHAR(64)    NOT NULL COMMENT 'SHA-256(明文) 的十六进制',
  PRIMARY KEY (ID),
  UNIQUE KEY uk_user_username (USERNAME)
) ENGINE = InnoDB DEFAULT CHARSET = utf8mb4 COMMENT = '系统用户';

-- -----------------------------------------------------------------------------
-- 2. 科室表（被医生表引用）
-- -----------------------------------------------------------------------------
CREATE TABLE IF NOT EXISTS Department (
  ID    VARCHAR(8)  NOT NULL COMMENT '科室编号 DEP01…',
  NAME  VARCHAR(30) NOT NULL COMMENT '科室名称',
  PRIMARY KEY (ID),
  UNIQUE KEY uk_dept_name (NAME)
) ENGINE = InnoDB DEFAULT CHARSET = utf8mb4 COMMENT = '科室';

-- -----------------------------------------------------------------------------
-- 3. 医生表（DEPARTMENT_ID 外键 -> Department.ID）
-- -----------------------------------------------------------------------------
CREATE TABLE IF NOT EXISTS Doctor (
  ID             VARCHAR(32) NOT NULL COMMENT '医生ID（UUID）',
  EMPLOYEE_NO    VARCHAR(20) NOT NULL COMMENT '工号',
  NAME           VARCHAR(20) NOT NULL COMMENT '姓名',
  DEPARTMENT_ID  VARCHAR(8)  NOT NULL COMMENT '所属科室',
  PRIMARY KEY (ID),
  UNIQUE KEY uk_doctor_empno (EMPLOYEE_NO),
  KEY idx_doctor_dept (DEPARTMENT_ID),
  KEY idx_doctor_name (NAME),
  CONSTRAINT fk_doctor_dept FOREIGN KEY (DEPARTMENT_ID)
    REFERENCES Department (ID)
    ON UPDATE CASCADE
    ON DELETE RESTRICT
) ENGINE = InnoDB DEFAULT CHARSET = utf8mb4 COMMENT = '医生';

-- -----------------------------------------------------------------------------
-- 4. 患者表（核心表：身份证唯一、字段长度按实际需要设置）
-- -----------------------------------------------------------------------------
CREATE TABLE IF NOT EXISTS Patient (
  ID                VARCHAR(32)  NOT NULL COMMENT '患者ID（UUID）',
  ID_CARD           CHAR(18)     NOT NULL COMMENT '身份证号',
  NAME              VARCHAR(20)  NOT NULL COMMENT '姓名',
  SEX               TINYINT      NOT NULL DEFAULT 0 COMMENT '0未知 1男 2女',
  DOB               DATE         NULL COMMENT '出生日期',
  HEIGHT            DECIMAL(5,1) NULL COMMENT '身高 cm',
  WEIGHT            DECIMAL(5,1) NULL COMMENT '体重 kg',
  MOBILEPHONE       CHAR(11)     NULL COMMENT '手机号',
  AGE               TINYINT UNSIGNED NULL COMMENT '年龄（由 DOB 计算写入）',
  CREATEDTIMESTAMP  DATETIME     NOT NULL DEFAULT CURRENT_TIMESTAMP COMMENT '建档时间',
  PRIMARY KEY (ID),
  UNIQUE KEY uk_patient_idcard (ID_CARD),
  KEY idx_patient_name (NAME),
  KEY idx_patient_phone (MOBILEPHONE),
  CONSTRAINT ck_patient_sex    CHECK (SEX IN (0, 1, 2)),
  CONSTRAINT ck_patient_height CHECK (HEIGHT IS NULL OR HEIGHT BETWEEN 30 AND 250),
  CONSTRAINT ck_patient_weight CHECK (WEIGHT IS NULL OR WEIGHT BETWEEN 2 AND 300),
  CONSTRAINT ck_patient_age    CHECK (AGE IS NULL OR AGE BETWEEN 0 AND 150)
) ENGINE = InnoDB DEFAULT CHARSET = utf8mb4 COMMENT = '患者';

-- -----------------------------------------------------------------------------
-- 5. 操作日志表（只追加，保留审计链）
-- -----------------------------------------------------------------------------
CREATE TABLE IF NOT EXISTS History (
  ID           VARCHAR(32)  NOT NULL COMMENT '日志ID（UUID）',
  USER_ID      VARCHAR(32)  NOT NULL COMMENT '操作人',
  EVENT        VARCHAR(200) NOT NULL COMMENT '操作描述',
  `TIMESTAMP`  DATETIME     NOT NULL DEFAULT CURRENT_TIMESTAMP COMMENT '发生时间',
  PRIMARY KEY (ID),
  KEY idx_history_user_time (USER_ID, `TIMESTAMP`),
  KEY idx_history_time (`TIMESTAMP`),
  CONSTRAINT fk_history_user FOREIGN KEY (USER_ID)
    REFERENCES `User` (ID)
    ON UPDATE CASCADE
    ON DELETE RESTRICT
) ENGINE = InnoDB DEFAULT CHARSET = utf8mb4 COMMENT = '操作日志';

-- -----------------------------------------------------------------------------
-- 6. 视图（只读展示：性别中文化、年龄实时计算、日志关联操作人）
-- -----------------------------------------------------------------------------
CREATE OR REPLACE VIEW v_patient AS
SELECT p.ID,
       p.ID_CARD,
       p.NAME,
       p.SEX,
       CASE p.SEX WHEN 1 THEN '男' WHEN 2 THEN '女' ELSE '未知' END AS SEX_NAME,
       p.DOB,
       p.HEIGHT,
       p.WEIGHT,
       p.MOBILEPHONE,
       TIMESTAMPDIFF(YEAR, p.DOB, CURDATE()) AS AGE_CALC,
       p.AGE,
       p.CREATEDTIMESTAMP
FROM Patient p;

CREATE OR REPLACE VIEW v_history AS
SELECT h.ID,
       h.USER_ID,
       u.FULLNAME AS USER_NAME,
       h.EVENT,
       h.`TIMESTAMP`
FROM History h
LEFT JOIN `User` u ON u.ID = h.USER_ID;

-- -----------------------------------------------------------------------------
-- 7. 应用账号（低权限：只有 DML，没有 DDL）
--    程序使用该账号连接，root 仅用于执行本脚本
-- -----------------------------------------------------------------------------
CREATE USER IF NOT EXISTS 'hospital'@'localhost' IDENTIFIED BY 'Hospital@2026';
GRANT SELECT, INSERT, UPDATE, DELETE ON hospital.* TO 'hospital'@'localhost';
FLUSH PRIVILEGES;

-- -----------------------------------------------------------------------------
-- 8. 种子数据（口令为 SHA-256 十六进制）
--    admin  / 123456      -> 8d969eef6ecad3c29a3a629280e686cf0c3f5d5a86aff3ca12020c923adc6c92
--    doctor / doctor123   -> f348d5628621f3d8f59c8cabda0f8eb0aa7e0514a90be7571020b1336f26c113
-- -----------------------------------------------------------------------------
INSERT IGNORE INTO `User` (ID, FULLNAME, USERNAME, PASSWORD) VALUES
 ('U0001', '系统管理员', 'admin',
  '8d969eef6ecad3c29a3a629280e686cf0c3f5d5a86aff3ca12020c923adc6c92'),
 ('U0002', '张医生', 'doctor',
  'f348d5628621f3d8f59c8cabda0f8eb0aa7e0514a90be7571020b1336f26c113');

INSERT IGNORE INTO Department (ID, NAME) VALUES
 ('DEP01', '内科'),
 ('DEP02', '外科'),
 ('DEP03', '儿科'),
 ('DEP04', '医学检验科');

INSERT IGNORE INTO Doctor (ID, EMPLOYEE_NO, NAME, DEPARTMENT_ID) VALUES
 ('D0001', 'E2026001', '张伟', 'DEP01'),
 ('D0002', 'E2026002', '李娜', 'DEP02'),
 ('D0003', 'E2026003', '王强', 'DEP03'),
 ('D0004', 'E2026004', '刘敏', 'DEP04'),
 ('D0005', 'E2026005', '陈晨', 'DEP01');

INSERT IGNORE INTO Patient
 (ID, ID_CARD, NAME, SEX, DOB, HEIGHT, WEIGHT, MOBILEPHONE, AGE) VALUES
 ('P0001', '441900199003070011', '赵磊',   1, '1990-03-07', 175.0, 70.5, '13800138001', 36),
 ('P0002', '441900199211150022', '钱静',   2, '1992-11-15', 162.0, 52.0, '13800138002', 34),
 ('P0003', '441900198807230033', '孙浩',   1, '1988-07-23', 180.0, 82.0, '13800138003', 38),
 ('P0004', '441900200101090044', '李婷',   2, '2001-01-09', 158.0, 48.5, '13800138004', 25),
 ('P0005', '441900197505180055', '周敏',   2, '1975-05-18', 160.0, 60.0, '13800138005', 51),
 ('P0006', '441900199509300066', '吴强',   1, '1995-09-30', 172.0, 68.0, '13800138006', 31),
 ('P0007', '441900198212120077', '郑丽',   2, '1982-12-12', 165.0, 58.0, '13800138007', 43),
 ('P0008', '441900200306060088', '王鹏',   1, '2003-06-06', 178.0, 75.0, '13800138008', 23),
 ('P0009', '441900199808080099', '冯雪',   2, '1998-08-08', 168.0, 55.0, '13800138009', 28),
 ('P0010', '441900196505050101', '陈建国', 1, '1965-05-05', 170.0, 72.0, '13800138010', 61);

INSERT IGNORE INTO History (ID, USER_ID, EVENT, `TIMESTAMP`) VALUES
 ('H0001', 'U0001', '系统初始化：导入 10 条患者 / 5 名医生 / 4 个科室', DATE_SUB(NOW(), INTERVAL 1 DAY)),
 ('H0002', 'U0001', '登录系统', DATE_SUB(NOW(), INTERVAL 2 HOUR));

-- 年龄统一按当前日期校正一次，保证与 DOB 一致
UPDATE Patient
SET AGE = TIMESTAMPDIFF(YEAR, DOB, CURDATE())
WHERE DOB IS NOT NULL;

-- -----------------------------------------------------------------------------
-- 9. 自检（执行完可以直接截图）
-- -----------------------------------------------------------------------------
SELECT '表数量' AS 检查项, COUNT(*) AS 结果
FROM information_schema.tables
WHERE table_schema = 'hospital' AND table_type = 'BASE TABLE';

SELECT 'Patient' AS 表名, COUNT(*) AS 行数 FROM Patient
UNION ALL SELECT 'Doctor', COUNT(*) FROM Doctor
UNION ALL SELECT 'Department', COUNT(*) FROM Department
UNION ALL SELECT 'User', COUNT(*) FROM `User`
UNION ALL SELECT 'History', COUNT(*) FROM History;

SELECT ID, NAME, SEX_NAME, DOB, AGE, AGE_CALC, CREATEDTIMESTAMP
FROM v_patient
ORDER BY CREATEDTIMESTAMP DESC
LIMIT 5;
