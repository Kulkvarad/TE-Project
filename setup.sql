CREATE DATABASE Soldier_safety;

USE Soldier_safety;

CREATE TABLE sensor_data (
    id INT AUTO_INCREMENT PRIMARY KEY ,
     timestamp DATETIME DEFAULT CURRENT_TIMESTAMP,
     temperature FLOAT,
    humidity FLOAT,
	pulse int,
    landmine bool
);

CREATE TABLE auth_user (
    id INT AUTO_INCREMENT PRIMARY KEY,
    username VARCHAR(100),
    pass VARCHAR(100)
    
);
INSERT INTO auth_user(username, password) VALUES ('admin', '123');
-- imporant use Soldier_safety; 
delete from sensor_data where id>0;

