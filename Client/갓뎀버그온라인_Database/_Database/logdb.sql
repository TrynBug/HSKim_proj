CREATE DATABASE `logdb`;

CREATE TABLE `logdb`.`gamelog` 
(
  `no` BIGINT NOT NULL AUTO_INCREMENT,
  `logtime` DATETIME NULL DEFAULT CURRENT_TIMESTAMP,
  `accountno` BIGINT NOT NULL,
  `servername` CHAR(64) NOT NULL,
  `type`	INT NOT NULL,
  `code`	INT NOT NULL,
  `param1` BIGINT NULL,
  `param2` BIGINT NULL,
  `param3` BIGINT NULL,
  `param4` BIGINT NULL,
  `message` CHAR(255) NULL,
  
PRIMARY KEY (`no`) );


-- 이미 있으니 이는 생략 

CREATE TABLE `logdb`.`monitorlog` 
(
  `no` BIGINT NOT NULL AUTO_INCREMENT,
  `logtime`		DATETIME,
  `serverno`	INT NOT NULL,
  `type`		INT NOT NULL,
  `value`		BIGINT NULL,
  
PRIMARY KEY (`no`) );


