package com.sparc.notify

import android.Manifest
import android.content.Context
import android.content.DialogInterface
import android.content.pm.PackageManager
import android.os.Build
import android.os.Bundle
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.widget.EditText
import android.widget.TextView
import androidx.activity.result.contract.ActivityResultContracts
import androidx.appcompat.app.AlertDialog
import androidx.appcompat.app.AppCompatActivity
import androidx.core.app.ActivityCompat
import androidx.core.content.ContextCompat
import androidx.recyclerview.widget.LinearLayoutManager
import androidx.recyclerview.widget.RecyclerView
import com.google.android.material.floatingactionbutton.FloatingActionButton
import com.google.firebase.messaging.FirebaseMessaging
import androidx.core.view.ViewCompat
import androidx.core.view.WindowInsetsCompat

class MainActivity : AppCompatActivity() {
    private lateinit var adapter: PatientCodeAdapter
    private lateinit var prefs: android.content.SharedPreferences
    private val PREFS_KEY = "patient_codes"

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)
        ViewCompat.setOnApplyWindowInsetsListener(findViewById(R.id.main)) { v, insets ->
            val systemBars = insets.getInsets(WindowInsetsCompat.Type.systemBars())
            v.setPadding(systemBars.left, systemBars.top, systemBars.right, systemBars.bottom)
            insets
        }

        // Request notification permission for Android 13+
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
            if (ContextCompat.checkSelfPermission(this, Manifest.permission.POST_NOTIFICATIONS) != PackageManager.PERMISSION_GRANTED) {
                ActivityCompat.requestPermissions(this, arrayOf(Manifest.permission.POST_NOTIFICATIONS), 1001)
            }
        }

        prefs = getSharedPreferences("codes", Context.MODE_PRIVATE)
        val codeList = loadCodes().toMutableList()
        adapter = PatientCodeAdapter(codeList, ::removeCode)

        val recyclerView = findViewById<RecyclerView>(R.id.patientCodeList)
        recyclerView.layoutManager = LinearLayoutManager(this)
        recyclerView.adapter = adapter

        val fab = findViewById<FloatingActionButton>(R.id.addCodeFab)
        fab.setOnClickListener { showAddCodeDialog() }
    }

    private fun showAddCodeDialog() {
        val view = layoutInflater.inflate(R.layout.dialog_add_userid, null)
        val input = view.findViewById<EditText>(R.id.inputUserId)
        input.hint = getString(R.string.enter_patient_code)
        AlertDialog.Builder(this)
            .setView(view)
            .setPositiveButton(android.R.string.ok) { _: DialogInterface, _: Int ->
                val code = input.text.toString().trim()
                if (code.isNotEmpty()) {
                    addCode(code)
                }
            }
            .setNegativeButton(android.R.string.cancel, null)
            .show()
    }

    private fun addCode(code: String) {
        val codes = loadCodes().toMutableSet()
        if (codes.add(code)) {
            saveCodes(codes)
            adapter.addCode(code)
            FirebaseMessaging.getInstance().subscribeToTopic(code)
        }
    }

    private fun removeCode(code: String) {
        val codes = loadCodes().toMutableSet()
        if (codes.remove(code)) {
            saveCodes(codes)
            adapter.removeCode(code)
            FirebaseMessaging.getInstance().unsubscribeFromTopic(code)
        }
    }

    private fun loadCodes(): Set<String> = prefs.getStringSet(PREFS_KEY, emptySet()) ?: emptySet()
    private fun saveCodes(codes: Set<String>) = prefs.edit().putStringSet(PREFS_KEY, codes).apply()
}

class PatientCodeAdapter(
    private val codes: MutableList<String>,
    private val onRemove: (String) -> Unit
) : RecyclerView.Adapter<PatientCodeAdapter.CodeViewHolder>() {
    class CodeViewHolder(view: View) : RecyclerView.ViewHolder(view) {
        val codeText: TextView = view.findViewById(R.id.codeText)
        val removeText: TextView = view.findViewById(R.id.removeText)
    }

    override fun onCreateViewHolder(parent: ViewGroup, viewType: Int): CodeViewHolder {
        val view = LayoutInflater.from(parent.context).inflate(R.layout.patient_code_item, parent, false)
        return CodeViewHolder(view)
    }

    override fun onBindViewHolder(holder: CodeViewHolder, position: Int) {
        val code = codes[position]
        holder.codeText.text = code
        holder.removeText.setOnClickListener { onRemove(code) }
    }

    override fun getItemCount(): Int = codes.size

    fun addCode(code: String) {
        codes.add(code)
        notifyItemInserted(codes.size - 1)
    }

    fun removeCode(code: String) {
        val idx = codes.indexOf(code)
        if (idx != -1) {
            codes.removeAt(idx)
            notifyItemRemoved(idx)
        }
    }
}