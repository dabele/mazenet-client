import java.io.*;
import java.net.*;


public class test_server {

	/**
	 * @param args
	 * @throws IOException 
	 * @throws InterruptedException 
	 */
	public static void main(String[] args) throws IOException, InterruptedException {
		ServerSocket welcomeSocket = new ServerSocket(5000);
		
		while (true) {			
			final Socket clientSocket = welcomeSocket.accept();			
			
			Thread t = new Thread(new Runnable() {
				DataInputStream in;
				DataOutputStream out;
				
				public void run() {					
					try {
						in = new DataInputStream(clientSocket.getInputStream());
						out = new DataOutputStream(clientSocket.getOutputStream());
					
						while (true) {
							String s = recv();
							System.out.println(s);
							send(s);
							Thread.sleep(1000);
						}
					} catch (IOException e) {
						return;
					} catch (InterruptedException e) {
						return;
					}
				}	
				
				public String recv() throws IOException {
					byte[] buf = new byte[4];
					in.readFully(buf);		
					int length = 0;
					length |= (buf[3] & 0xff);
					length <<= 8;
					length |= (buf[2] & 0xff);
					length <<= 8;
					length |= (buf[1] & 0xff);
					length <<= 8;
					length |= (buf[0] & 0xff);
					
					buf = new byte[length];
					in.readFully(buf);
					String s = new String(buf);
					return s;					
				}
				
				public void send(String s) throws IOException {
					int length = s.length();
					byte[] buf = new byte[4];
					buf[0] = (byte)length;
					length >>= 8;
					buf[1] = (byte)length;
					length >>= 8;
					buf[2] = (byte)length;
					length >>= 8;
					buf[3] = (byte)length;
					
					out.write(buf);
					out.writeBytes(s);
				}
			});
			
			t.start();
		}
		
	}

}
