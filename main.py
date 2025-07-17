from flask import Flask, request, render_template, g, redirect, url_for, session
import pymysql
from flask_socketio import SocketIO, emit
app = Flask(__name__)
app.secret_key = 'your_secret_key'  # Change this to something secure
security_key='123@'
socketio = SocketIO(app)
# MySQL config
db_config = {
    'host': 'localhost',
    'user': 'root',
    'password': 'root',
    'database': 'Soldier_safety',
    'cursorclass': pymysql.cursors.DictCursor
}

def get_db():
    if 'db' not in g:
        g.db = pymysql.connect(**db_config)
    return g.db

@app.teardown_appcontext
def close_db(exception):
    db = g.pop('db', None)
    if db is not None:
        db.close()

@app.route('/')
def index():
    return render_template('login.html')

@app.route('/login', methods=['POST'])
def login():
    username = request.form.get('username')
    password = request.form.get('password')

    if not username or not password:
        return render_template('login.html', error="Missing credentials")

    conn = get_db()
    with conn.cursor() as cursor:
        cursor.execute("SELECT * FROM auth_user WHERE username=%s AND pass=%s", (username, password))
        user = cursor.fetchone()

    if user:
        session['user'] = username
        return redirect(url_for('dashboard'))
    else:
        return render_template('login.html', error="Invalid username or password")

@app.route('/logout')
def logout():
    session.pop('user', None)
    return redirect(url_for('index'))

@app.route('/dashboard', methods=['GET'])
def dashboard():
    if 'user' not in session:
        return redirect(url_for('index'))

    conn = get_db()
    with conn.cursor() as cursor:
        cursor.execute("SELECT * FROM sensor_data ORDER BY timestamp DESC LIMIT 10")
        data = cursor.fetchall()
    return render_template('dashboard.html', sensor_data=data)

@app.route('/insert', methods=['GET'])
def insert_data():
    temp = request.args.get('temperature')

    pulse = request.args.get('pulse')
    landmine = request.args.get('landmine')
    key=request.args.get('key')
    if security_key==key:
        if not temp  or not pulse or not landmine:
            return "Missing temperature or humidity", 400

        try:
            conn = get_db()
            with conn.cursor() as cursor:
                cursor.execute(
                    "INSERT INTO sensor_data (temperature, pulse, landmine) VALUES ( %s, %s, %s)",
                    (temp,  pulse, landmine)
                )
            conn.commit()
            socketio.emit('update_dashboard', {'message': 'success'})
            return "Data inserted", 200
        except Exception as e:
            return str(e), 500
    else :
        return "Invalid key", 400
if __name__ == '__main__':
    app.run(host='0.0.0.0', port=8080)
