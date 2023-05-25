/*
SQLyog Community v13.1.5  (64 bit)
MySQL - 5.7.40-0ubuntu0.18.04.1 : Database - piwxrx
*********************************************************************
*/

/*!40101 SET NAMES utf8 */;

/*!40101 SET SQL_MODE=''*/;

/*!40014 SET @OLD_UNIQUE_CHECKS=@@UNIQUE_CHECKS, UNIQUE_CHECKS=0 */;
/*!40014 SET @OLD_FOREIGN_KEY_CHECKS=@@FOREIGN_KEY_CHECKS, FOREIGN_KEY_CHECKS=0 */;
/*!40101 SET @OLD_SQL_MODE=@@SQL_MODE, SQL_MODE='NO_AUTO_VALUE_ON_ZERO' */;
/*!40111 SET @OLD_SQL_NOTES=@@SQL_NOTES, SQL_NOTES=0 */;
/*Table structure for table `areanames` */

DROP TABLE IF EXISTS `areanames`;

CREATE TABLE `areanames` (
  `AreaCode` varchar(16) NOT NULL,
  `AreaName` varchar(64) DEFAULT NULL,
  PRIMARY KEY (`AreaCode`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8;

/*Table structure for table `locations` */

DROP TABLE IF EXISTS `locations`;

CREATE TABLE `locations` (
  `LocationCode` varchar(16) DEFAULT NULL,
  `LocationName` varchar(64) DEFAULT NULL
) ENGINE=MyISAM DEFAULT CHARSET=utf8;

/*Table structure for table `receivers` */

DROP TABLE IF EXISTS `receivers`;

CREATE TABLE `receivers` (
  `callsign` varchar(32) DEFAULT NULL,
  `location` varchar(32) DEFAULT NULL,
  `frequency` double DEFAULT NULL,
  `latitude` double DEFAULT NULL,
  `longitude` double DEFAULT NULL,
  `active` int(1) DEFAULT '0'
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

/*Table structure for table `roles` */

DROP TABLE IF EXISTS `roles`;

CREATE TABLE `roles` (
  `callsign` varchar(15) NOT NULL,
  `role` varchar(15) NOT NULL DEFAULT 'USER',
  PRIMARY KEY (`callsign`,`role`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

/*Table structure for table `samemessages` */

DROP TABLE IF EXISTS `samemessages`;

CREATE TABLE `samemessages` (
  `Originator` varchar(32) DEFAULT NULL,
  `Posttime` datetime DEFAULT NULL,
  `Agency` varchar(32) DEFAULT NULL,
  `Bulletin` varchar(256) DEFAULT NULL,
  `TimeofIssue` varchar(32) DEFAULT NULL,
  `DateofIssue` varchar(32) DEFAULT NULL,
  `PurgeTime` varchar(32) DEFAULT NULL,
  `Areas` varchar(256) DEFAULT NULL
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

/*Table structure for table `subscriptions` */

DROP TABLE IF EXISTS `subscriptions`;

CREATE TABLE `subscriptions` (
  `callsign` varchar(32) NOT NULL,
  `areacode` varchar(16) NOT NULL
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

/*Table structure for table `users` */

DROP TABLE IF EXISTS `users`;

CREATE TABLE `users` (
  `callsign` varchar(15) NOT NULL,
  `password` varchar(32) NOT NULL,
  `firstname` varchar(16) DEFAULT NULL,
  `lastname` varchar(32) DEFAULT NULL,
  `email` varchar(64) DEFAULT NULL,
  `meshphone` varchar(8) DEFAULT NULL,
  `flags` int(4) DEFAULT '0',
  PRIMARY KEY (`callsign`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

/*!40101 SET SQL_MODE=@OLD_SQL_MODE */;
/*!40014 SET FOREIGN_KEY_CHECKS=@OLD_FOREIGN_KEY_CHECKS */;
/*!40014 SET UNIQUE_CHECKS=@OLD_UNIQUE_CHECKS */;
/*!40111 SET SQL_NOTES=@OLD_SQL_NOTES */;
