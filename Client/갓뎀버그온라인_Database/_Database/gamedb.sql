CREATE DATABASE `gamedb`;

CREATE TABLE `gamedb`.`character` 
(
  `accountno` BIGINT NOT NULL,
  `charactertype` INT NOT NULL,
  `posx` FLOAT NOT NULL,
  `posy` FLOAT NOT NULL,
  `tilex` INT NOT NULL,
  `tiley` INT NOT NULL,
  `rotation` INT NOT NULL,
  `crystal` INT NOT NULL,
  `hp` INT NOT NULL,
  `exp` BIGINT NOT NULL,    -- 미사용
  `level` INT NOT NULL,     -- 미사용
  `die` INT NOT NULL DEFAULT 0,   -- 죽은 상태로 접속 종료시 1 

PRIMARY KEY (`accountno`));



CREATE TABLE `gamedb`.`data_monster` 
(
  `monster_type` BIGINT NOT NULL,		-- 몬스터 타입 1 한개
  `hp` INT NOT NULL,					-- 몬스터 생성 HP
  `damage` INT NOT NULL					-- 몬스터 공격 데미지 수치
);

INSERT INTO `gamedb`.`data_monster` (`monster_type`, `hp`, `damage`) VALUES (1, 100, 298);



CREATE TABLE `gamedb`.`data_crystal` 
(
  `crystal_type` BIGINT NOT NULL,		-- 크리스탈 타입 1,2,3
  `amount` INT NOT NULL					-- 해당 크리스탈의 크리스탈 수치
);  

INSERT INTO `gamedb`.`data_crystal` (`crystal_type`, `amount`) VALUES (1, 100);
INSERT INTO `gamedb`.`data_crystal` (`crystal_type`, `amount`) VALUES (2, 200);
INSERT INTO `gamedb`.`data_crystal` (`crystal_type`, `amount`) VALUES (3, 500);



CREATE TABLE `gamedb`.`data_player` 
(
  `damage` BIGINT NOT NULL,				-- 플레이어 공격 데미지 수치
  `hp` INT NOT NULL,			    		-- HP 초기치
  `recovery_hp` INT NOT NULL			-- 앉기 동작시 HP 회복량 / 초당
);  

INSERT INTO `gamedb`.`data_player` (`damage`, `hp`, `recovery_hp`) VALUES (25, 5000, 5);