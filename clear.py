import requests
import datetime
import sqlite3
import sys
sys.path.append("__HOME__/finalProject")

def request_handler(request):


	example_db = "__HOME__/finalProject/players.db" # just come up with name of database
	conn = sqlite3.connect(example_db)  # connect to that database (will create if it doesn't already exist)
	c = conn.cursor()  # make cursor into database (allows us to execute commands)
	c.execute('''DROP TABLE IF EXISTS playerData ;''') # run a CREATE TABLE command
	c.execute('''DROP TABLE IF EXISTS lobbyPlayers ;''') # run a CREATE TABLE command
	conn.commit() # commit commands
	conn.close() # close connection to database
