CREATE TABLE IF NOT EXISTS `character_druid_forms_scales` (
  `guid` INT UNSIGNED NOT NULL COMMENT 'GUID del personaje',
  `form` TINYINT UNSIGNED NOT NULL COMMENT 'ID de la forma de Druida (FORM_BEAR, FORM_CAT, etc.)',
  `scale` FLOAT NOT NULL DEFAULT 1.0 COMMENT 'Escala asignada a la forma',
  PRIMARY KEY (`guid`, `form`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;