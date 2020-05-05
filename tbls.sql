CREATE TABLE `history` (  `id` bigint(20) NOT NULL AUTO_INCREMENT,  `name` text CHARACTER SET latin1,  `path` text CHARACTER SET latin1,  PRIMARY KEY (`id`)) ENGINE=InnoDB AUTO_INCREMENT=1 DEFAULT CHARSET=utf8;

CREATE TABLE `usr` (  `id` bigint(20) NOT NULL AUTO_INCREMENT,  `name` text CHARACTER SET latin1,  `pwd` text CHARACTER SET latin1,  `admin` int(11) DEFAULT NULL COMMENT '0:不是admin,1:是admin',  PRIMARY KEY (`id`)) ENGINE=InnoDB AUTO_INCREMENT=1 DEFAULT CHARSET=utf8;
