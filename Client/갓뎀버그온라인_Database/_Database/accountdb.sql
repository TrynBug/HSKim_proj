CREATE DATABASE `accountdb`;



CREATE TABLE `accountdb`.`account` (
	`accountno` BIGINT NOT NULL AUTO_INCREMENT,
	`userid` CHAR(64) NOT NULL,
	`userpass` CHAR(64) NOT NULL,
	`usernick` CHAR(64) NOT NULL,	
	PRIMARY KEY (`accountno`),
	INDEX `userid_INDEX` (`userid` ASC)
);


-- 실제 회원은 accountno 가 1,000,000 부터 시작 하기로 한다
-- 게스트 계정은 accountno 가 2,000,000 부터 시작 한다.
-- 1 ~ 999,999 은 테스트용 더미계정으로 사용.

ALTER TABLE `accountdb`.`account` auto_increment = 2000000; 
 
 
CREATE TABLE `accountdb`.`sessionkey` (
	`accountno` BIGINT NOT NULL,
	`sessionkey` CHAR(64) NULL,
    PRIMARY KEY (`accountno`)
);

	
CREATE TABLE `accountdb`.`status` (
	`accountno` BIGINT NOT NULL,
	`status` INT NOT NULL DEFAULT 0,
	PRIMARY KEY (`accountno`)
);

	
CREATE TABLE `accountdb`.`guestno` (
	`no` BIGINT NOT NULL AUTO_INCREMENT,
    PRIMARY KEY (`no`)
);
	

CREATE TABLE `accountdb`.`whiteip` (
	`no` BIGINT NOT NULL AUTO_INCREMENT,
	`ip` CHAR(32) NOT NULL,
    PRIMARY KEY (`no`)
);
	
	
CREATE
    VIEW `accountdb`.`v_account` 
    AS
(
SELECT
  `a`.`accountno`  AS `accountno`,
  `a`.`userid`     AS `userid`,
  `a`.`usernick`   AS `usernick`,
  `b`.`sessionkey` AS `sessionkey`,
  `c`.`status`     AS `status`
FROM `account` `a`
 LEFT JOIN `sessionkey` `b` ON (`a`.`accountno` = `b`.`accountno`)
 LEFT JOIN `status` `c` ON (`a`.`accountno` = `c`.`accountno`)
);