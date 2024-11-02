package com.example.wificar;

import androidx.appcompat.app.AppCompatActivity;
import android.annotation.SuppressLint;
import android.content.Intent;
import android.os.Bundle;
import android.os.StrictMode;
import android.view.View;
import android.widget.Button;
import android.widget.EditText;
import android.widget.TextView;
import java.io.IOException;
import java.net.InetAddress;
import java.net.Socket;
import java.net.UnknownHostException;


public class MainActivity extends AppCompatActivity {
    Button connect, disconnect, start;
    private static Socket sock = null;

    public static synchronized Socket getSocket(){
        return sock;
    }
    TextView textView;
    EditText numar_port, adresa_ip;
    @SuppressLint("ClickableViewAccessibility")
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        // se incarca fisierul xml asociat activitatii
        setContentView(R.layout.activity_main);
        // se obtin referintele catre metodele de interfata grafica
        connect = findViewById(R.id.connect);
        disconnect = findViewById(R.id.disconnect);
        numar_port = findViewById(R.id.numar_port);
        adresa_ip = findViewById(R.id.adresa_ip);
        start = findViewById(R.id.start);
        textView = findViewById(R.id.textView);

        // se lanseaza o noua intentie pentru pornirea activitatii SecondActivity

        start.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                startActivity(new Intent(MainActivity.this, SecondActivity.class));
            }
        });



        connect.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                //se activeaza modul strict de executare
                enableStrictMode();
                //se creeaza un fir de executie separat
                Thread connectionThread = new Thread(new Runnable() {
                    @Override
                    public void run() {
                        if (sock == null) {
                            try {
                                //se verifica daca au fost introduse adresa IP si portul
                                if(adresa_ip.getText().length()!=0 && numar_port.getText().length()!=0) {
                                    // se creeaza un obiect de tip Socket
                                    // se preiau din EditText adresa IP si portul
                                    // si se transmit in constructorul obiectului
                                    sock = new Socket(InetAddress.getByName(adresa_ip.getText().toString()), Integer.parseInt(numar_port.getText().toString()));
                                    // se afiseaza in textView mesajul "Conexiune reusita!"
                                    textView.setText("Conexiune reușită!");
                                    //daca denumirea pentru gazda nu poate fi rezolvata de serverul DNS
                                }
                            } catch (UnknownHostException e) {
                                e.printStackTrace();
                                //daca conexiunea nu a fost acceptata de catre server
                            } catch (IOException e) {
                                e.printStackTrace();
                            }
                        }

                        runOnUiThread(new Runnable() {
                            @Override
                            public void run() {
                                //daca conexiunea nu a putut fi realizatat
                                if(sock==null){
                                    textView.setText("Conexiune eșuată!");
                                }
                                else{
                                    textView.setText("Conexiune reușită!");
                                }

                            }
                        });
                    }
                });
                //se porneste firul de executie
                connectionThread.start();
            }
        });



        disconnect.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                //daca conexiunea este activa
                if (sock != null) {
                    try {
                        //se inchide socket-ul prin apelul metodei close()
                        sock.close();
                        //se seteaza sock la null
                        sock = null;
                        // se afiseaza in textView conexiune indisponibila
                        textView.setText("Conexiune indisponibilă!");
                    } catch (IOException e) {
                        e.printStackTrace();
                    }
                }
            }
        });
    }

    public void enableStrictMode()
    {
        StrictMode.ThreadPolicy policy = new StrictMode.ThreadPolicy.Builder().permitAll().build();

        StrictMode.setThreadPolicy(policy);
    }

}








