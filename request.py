import requests
import datetime
import sqlite3
import sys
sys.path.append("__HOME__/finalProject")

def request_handler(request):
	
	if request["method"] == "GET":
		example_db = "__HOME__/finalProject/players.db" # just come up with name of database
		conn = sqlite3.connect(example_db)  # connect to that database (will create if it doesn't already exist)
		c = conn.cursor()  # make cursor into database (allows us to execute commands)
		c.execute('''CREATE TABLE IF NOT EXISTS playerData (ID int, health int);''') # run a CREATE TABLE command
		things =  c.execute('''SELECT * FROM playerData;''').fetchall()
		conn.commit() # commit commands
		conn.close() # close connection to databa
		return things


	if request["method"] == "POST":

		data = request["form"]
		ID = int(data["ID"])
		health = int(data["health"])
		action = data["action"]

		example_db = "__HOME__/finalProject/players.db" # just come up with name of database
		conn = sqlite3.connect(example_db)  # connect to that database (will create if it doesn't already exist)
		c = conn.cursor()  # make cursor into database (allows us to execute commands)
		c.execute('''CREATE TABLE IF NOT EXISTS playerData (ID int, health int);''') # run a CREATE TABLE command
		if(action == "initial"):
			things =  c.execute('''SELECT * FROM playerData;''').fetchall()
			if not (ID,health) in things:
				c.execute('''INSERT into playerData VALUES (?,?);''',(ID,health))
		else:

			prev = c.execute('''SELECT * FROM playerData WHERE ID=(?);''',(ID))

			c.execute('''DELETE FROM playerData WHERE ID=(?);''',(ID))

			c.execute('''INSERT into playerData VALUES (?,?);''',(ID,prev[1]-1))

		conn.commit() # commit commands
		conn.close() # close connection to databa
		return str((ID,health)) + " inserted"

	# {'method': 'POST', 'args': [], 'values': {}, 'is_json': False, 'form': {'ID': '1', 'health': '5', 'action': 'initial'}}
	return request
