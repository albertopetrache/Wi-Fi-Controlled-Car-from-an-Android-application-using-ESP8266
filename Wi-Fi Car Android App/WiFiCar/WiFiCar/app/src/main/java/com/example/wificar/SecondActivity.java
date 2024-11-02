package com.example.wificar;

import android.os.Bundle;
import android.view.View;
import android.widget.Button;
import android.widget.ImageButton;
import androidx.appcompat.app.AppCompatActivity;
import java.io.BufferedOutputStream;
import java.io.IOException;
import java.io.PrintWriter;

public class SecondActivity extends AppCompatActivity {
    ImageButton inainte, inapoi, stanga, dreapta;
    Button avarii, faruri, sem_st, sem_dr, claxon, speedup, slowdown, frana;

    public void transmiteMesaj(String comanda) {
        if (MainActivity.getSocket() != null) {
            try {
                BufferedOutputStream bufferedOutputStream = new BufferedOutputStream(MainActivity.getSocket().getOutputStream());
                PrintWriter printWriter = new PrintWriter(bufferedOutputStream, true);
                printWriter.println(comanda);
            } catch (IOException e) {
                e.printStackTrace();
            }
        }
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        // se incarca fisierul xml asociat activitatii
        setContentView(R.layout.activity_second);
        // se obtin referintele catre elementele de interfata grafica
        inainte = findViewById(R.id.inainte);
        inapoi = findViewById(R.id.inapoi);
        stanga = findViewById(R.id.stanga);
        dreapta = findViewById(R.id.dreapta);
        avarii = findViewById(R.id.avarii);
        faruri = findViewById(R.id.faruri);
        sem_st = findViewById(R.id.sem_st);
        sem_dr = findViewById(R.id.sem_dr);
        claxon = findViewById(R.id.claxon);
        speedup = findViewById(R.id.speedup);
        slowdown = findViewById(R.id.slowdown);
        frana = findViewById(R.id.frana);

        inainte.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                transmiteMesaj("inainte");
            }
        });

        inapoi.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                transmiteMesaj("inapoi");
            }
        });

        stanga.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                transmiteMesaj("stanga");
            }
        });

        dreapta.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                transmiteMesaj("dreapta");
            }
        });

        sem_st.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                transmiteMesaj("sem_st");
            }
        });

        sem_dr.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                transmiteMesaj("sem_dr");
            }
        });

        avarii.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                transmiteMesaj("avarii");
            }
        });

        faruri.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                transmiteMesaj("faruri");
            }
        });

        claxon.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                transmiteMesaj("claxon");
            }
        });

        speedup.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                transmiteMesaj("speedup");
            }
        });

        slowdown.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                transmiteMesaj("slowdown");
            }
        });

        frana.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                transmiteMesaj("frana");
            }
        });



    }
}
