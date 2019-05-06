import datetime
import sqlite3
import sys
import json
sys.path.append("__HOME__/finalProject")

def handleWaiting(c):
    '''
    This request returns the seconds left until the game starts
    '''

    c.execute('''CREATE TABLE IF NOT EXISTS lobbyPlayers (user text, time timestamp);''') # run a CREATE TABLE command

    players_dict = {}
    players =  c.execute('''SELECT * FROM lobbyPlayers ORDER BY time DESC;''').fetchall()

    # make sure player entries are unique
    for db_data in players:
        if db_data[0] not in players_dict:
            players_dict[db_data[0]] = db_data[1]

    pl = sorted(players_dict.items(), key=lambda x: x[1], reverse=True) # sort by time
    if len(pl) >= 2:    # we have at least 2 players
        # convert datetime strings to datetime obj
        most_recent_time = datetime.datetime.strptime(pl[0][1], "%Y-%m-%d %H:%M:%S.%f")
        if (datetime.datetime.now()-most_recent_time).total_seconds() >= 20:    # check if 20 seconds have elapsed
            # game starts
            for player in pl:
                c.execute('''INSERT into playerData VALUES (?,?,?,?);''', (player[0], 3,0,0))   # players will be initialized to 3 health
            c.execute('''DROP TABLE IF EXISTS lobbyPlayers''')  # reset the lobby players, because we don't need them anymore at this point
            return 0    # let players know game has started
        else:
            return 20-round((datetime.datetime.now()-most_recent_time).total_seconds()) # players are still being added in
    elif len(pl) == 0:
        return 0    # we're guaranteed that players send a post request first then
    return 20   # in the case of one player

def request_handler(request):
    # every request should have a user and an action
    if request["method"] == "GET":
        try:
            data = request['values']
            action = data['action']
        except:
            return "Wrong GET Request\n" + str(request)

        example_db = "__HOME__/finalProject/players.db" # just come up with name of database
        conn = sqlite3.connect(example_db)  # connect to that database (will create if it doesn't already exist)
        c = conn.cursor()  # make cursor into database (allows us to execute commands)
        c.execute('''CREATE TABLE IF NOT EXISTS playerData (user text, health int, lat float, long float, kills, float);''') # run a CREATE TABLE command

        if action == "waiting":
            # displays how long before the game begins if there is at least 2 players in the game
            g = handleWaiting(c)
            conn.commit() # commit commands
            conn.close() # close connection to database
            return "Game Started!" if g == 0 else "Game starts in " + str(g) + " seconds"

        if (action == "getPlayers"):
            players =  c.execute('''SELECT user FROM playerData;''').fetchall()
            ret = []
            for el in players:
                ret.append(el[0])

            return json.dumps(ret)

        if (action == "getAll"):
            players = c.execute('''SELECT * FROM playerData;''').fetchall()
            ret = {}   # Dictionary of {player: [Life, lat, lon]}
            for player in players:
                temp = player[1:]
                ret[player[0]] = temp
            return json.dumps(ret)

        if(action == "display"):
            # action to help debug it displays all the players in the player data
            things =  c.execute('''SELECT * FROM playerData;''').fetchall()
            conn.commit() # commit commands
            conn.close() # close connection to database
            return things
        if(action == "getGPS"):
            # action to get all the GPS data for all the players
            things =  c.execute('''SELECT * FROM playerData;''').fetchall()
            gps = []
            for each in things:
                gps+=[(each[2],each[3])]
            conn.commit() # commit commands
            conn.close() # close connection to database
            return gps
        if(action == "getHealth"):
            # action to get all the GPS data for all the players
            things =  c.execute('''SELECT * FROM playerData;''').fetchall()
            health = None
            for each in things:
                if(each[0] == data["user"]):
                    health = each[1]
                    break
            if(health == None):
                conn.commit() # commit commands
                conn.close()
                return "That player does not exist"

            conn.commit() # commit commands
            conn.close() # close connection to database
            return health
        if action == "gameStatus":

            # returns wether a player has won or how many players are left in the game
            things =  c.execute('''SELECT * FROM playerData;''').fetchall()
            if len(things) == 1:
                # displays who won
                conn.commit() # commit commands
                conn.close() # close connection to database

                return str(things[0][0]) + " has won the game!"

            conn.commit() # commit commands
            conn.close() # close connection to database
            return "There are "+str(len(things))+" players remaining"
    if request["method"] == "POST":
        try:
            data = request["form"]
            action = data["action"]
        except:
            return "Incorrect POST request\n" + str(request)

        example_db = "__HOME__/finalProject/players.db"
        conn = sqlite3.connect(example_db)  # connect to that database (will create if it doesn't already exist)
        c = conn.cursor()  # make cursor into database (allows us to execute commands)
        c.execute('''CREATE TABLE IF NOT EXISTS playerData (user text, health int,lat float, lon float, kills float);''') # run a CREATE TABLE command

        # handle specific POST Requests
        if (action == "initial"):
            # players ready up and are handled according to when the last player readyd up
            g = handleStartGame(c, data)
            conn.commit() # commit commands
            conn.close() # close connection to database
            return g
        if action == "shot":
            # # handles the health of each player and removes playes who are dead
            user = data["user"]
            origin = data["origin"]
            things =  c.execute('''SELECT * FROM playerData;''').fetchall()
            currentKills = None
            playerHealth = None
            # finds the health of the specific player
            for each in things:
                if each[0] == user:
                    playerHealth = each[1]
                if each[0] == origin:
                    currentKills = each[4]
            if currentKills == None:
                return "That killer doesnt exist"
            if playerHealth == None:
                # check to see if its valid
            	return "That player does not exist"

            playerHealth -= 1

            # updates the table to reflect current health
            c.execute('''UPDATE playerData SET health = (?) WHERE user = (?);''',(playerHealth,user))
            if(playerHealth <= 0):
                # if the update leaves a player with no health they are removed form the table
                c.execute('''DELETE FROM playerData WHERE user = ?;''',(user,))

                things =  c.execute('''SELECT * FROM playerData;''').fetchall()
                currentKills += 1
                c.execute('''UPDATE playerData SET kills = (?) WHERE user = (?);''',(currentKills,origin))

                conn.commit()
                conn.close()
                return str(user) + " has died"

            conn.commit()
            conn.close()
            return str(user) + " took damage"
        if action == "gpsUpdate":
            user = data["user"]
            lat = float(data["lat"])
            lon = float(data["lon"])

            # update player locations
            c.execute('''UPDATE playerData SET lat=(?), lon=(?) WHERE user = (?);''',(lat,lon,user))

            c.execute('''UPDATE playerData SET lat = (?) WHERE user = (?);''',(lat,user))
            c.execute('''UPDATE playerData SET lon = (?) WHERE user = (?);''',(lon,user))

            # return location of all players
            locs =  c.execute('''SELECT user, lat, lon FROM playerData;''').fetchall()
            # construct JSON Response
            ret_locs = {}
            for i in locs:
                ret_locs[i[0]] = (i[1], i[2])

            ret_locs = json.dumps(ret_locs) # create array into JSON
            conn.commit()
            conn.close()
            return ret_locs

    return request

def handleStartGame(c, data):
    try:
        user = data['user']
    except:
        return "User not provided!"

    c.execute('''CREATE TABLE IF NOT EXISTS lobbyPlayers (user text, time timestamp);''') # run a CREATE TABLE command
    data = things =  c.execute('''SELECT * FROM lobbyPlayers;''').fetchall()

    for i in range(len(data)):
        data[i] = data[i][0]
    if not user in data:
        c.execute('''INSERT into lobbyPlayers VALUES (?,?);''', (user, datetime.datetime.now()))
    else:
        c.execute('''UPDATE lobbyPlayers SET time = (?) WHERE user = (?);''',(datetime.datetime.now(),user))

    # c.exceptecute('''INSERT into lobbyPlayers VALUES (?,?);''', (user, datetime.datetime.now()))

    players =  c.execute('''SELECT * FROM lobbyPlayers ORDER BY time DESC;''').fetchall()

    things = ""
    for player in players:
        things += str(player) + "\n"

    return things
