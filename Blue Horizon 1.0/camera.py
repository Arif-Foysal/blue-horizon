import cv2
import datetime
from flask import Flask, render_template, Response, send_file, redirect, url_for

video = cv2.VideoCapture(0)
app = Flask(__name__)

def video_stream():
    while True:
        ret, frame = video.read()
        if not ret:
            break
        else:
            ret, buffer = cv2.imencode('.jpeg', frame)
            frame = buffer.tobytes()
            yield (b'--frame\r\n'
                   b'Content-type: image/jpeg\r\n\r\n' + frame + b'\r\n\r\n')

@app.route('/camera')
def camera():
    return render_template('camera.html')

@app.route('/video_feed')
def video_feed():
    return Response(video_stream(), mimetype='multipart/x-mixed-replace; boundary=frame')

@app.route('/capture_image')
def capture_image():
    # Capture an image
    ret, frame = video.read()

    # Encode the image as JPEG
    _, buffer = cv2.imencode('.jpeg', frame)
    image_data = buffer.tobytes()

    # Generate a unique filename based on the current timestamp
    timestamp = datetime.datetime.now().strftime("%Y%m%d%H%M%S")
    filename = f"captured_image_{timestamp}.jpg"

    # Save the image
    cv2.imwrite(filename, frame)

    # Redirect to the captured image
    return redirect(url_for('download_image', filename=filename))

@app.route('/download_image/<filename>')
def download_image(filename):
    return send_file(filename, as_attachment=True)

if __name__ == "__main__":
    app.run(host='0.0.0.0', port='5000', debug=False)
