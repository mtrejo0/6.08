import sqlite3

def request_handler(request):
    if request['method']=='GET':
        return r"""
Vote on who will win Game 6?
<form action="" method="post">
    <button name="team" value="mapleleafs">Mapleleafs</button>
    <button name="team" value="bruins">Bruins</button><br>
</form>
"""
    elif request['method']=='POST':
        game_db = "__HOME__/game.db" # just come up with name of database
        conn = sqlite3.connect(game_db)  # connect to that database (will create if it doesn't already exist)
        c = conn.cursor()  # make cursor into database (allows us to execute commands)
        c.execute('''CREATE TABLE IF NOT EXISTS game_table (id integer primary key AUTOINCREMENT, team text);''') # run a CREATE TABLE command
        c.execute('''INSERT into game_table (team) VALUES (?);''',(request['form']['team'],))
        mp = c.execute('''SELECT * FROM game_table WHERE team = ? ORDER BY id ASC;''', ('mapleleafs',)).fetchall()
        mp = len(mp)
        br = c.execute('''SELECT * FROM game_table WHERE team = ? ORDER BY id ASC;''', ('bruins',)).fetchall()
        br = len(br)
        conn.commit() # commit commands
        conn.close() # close connection to database
        return r'''
Current Tally:<br>
<ul>
<li>MapleLeafs: {} votes</li>
<li>Bruins: {} votes</li>
</ul>
<a href="http://iesc-s1.mit.edu/608dev/sandbox/moisest/form_submit_ex.py">Click here to vote again</a>
'''.format(mp,br)
