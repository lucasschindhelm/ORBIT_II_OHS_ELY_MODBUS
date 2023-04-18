#include <stdio.h>
#include <stdlib.h>
#include <modbus.h>
#include <time.h>
#include <math.h>
//#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <signal.h>
#include <string.h>

#define READ_START 0x8060
#define READ_END 0x8075
#define WRITE_START 0x8000
#define WRITE_END 0x8002
#define MAX_CURRENT 52
#define MIN_CURRENT 15
#define TSPUELEN 300

modbus_t *ctx_ELY=NULL;
long zeitstempel_spuelen=0;
char pfad[100]="./ELY_results/";


void sigfunc(int sig){
       	//fprintf(stderr, "%d\n", sig);
	if (ctx_ELY!=NULL){
		modbus_close(ctx_ELY);
		modbus_free(ctx_ELY);
	}
	 exit(EXIT_FAILURE);
}
void check_signal(){                                                                                                         signal(SIGABRT,sigfunc);
		signal(SIGILL,sigfunc);
        signal(SIGINT,sigfunc);
        signal(SIGSEGV,sigfunc);
        signal(SIGTERM,sigfunc);
}

void change_byte_order(uint16_t *werte, int nreg){
	int k;
	uint8_t a,b;
	for(k=0; k< nreg;k++){
		a= (werte[k]>>8) & 0xFF;
		b= werte[k] & 0xFF;
		werte[k]=b<<8 | a;
		
	}
	
}
int get_time() {
        long Jetzt;
        time(&Jetzt);
        return Jetzt;
}


long get_time_string(char datum[100], int size_datum) {
        time_t buf[1];
        time_t *tp1=&buf[0];
        time(tp1);
        struct tm buf1[1];
        struct tm *tp;
        tp=localtime(tp1);
        int tag=tp->tm_mday;
        int monat=tp->tm_mon+1;
        int jahr=tp->tm_year+1900;
        int stunde=tp->tm_hour;
        int minuten=tp->tm_min;
        int sekunden=tp->tm_sec;
        snprintf(datum,size_datum,"%4d%02d%02d_%02d%02d%02d",jahr,monat, tag, stunde,minuten, sekunden);
        return (long) buf[0];
}
void create_result_directory(char *pfad_init){
        char datum[100];
        long datesec=get_time_string(datum, sizeof(datum));
        strcat(pfad_init, datum);
        char kommando[100]="sudo mkdir -p ";
        strcat(kommando, pfad_init);
       	FILE *Datei= popen(kommando,"r");
		pclose(Datei);
}



uint16_t read_status_word1(uint16_t  statuswort, int order){
	uint16_t error=0;
	if (order==0){
		error=statuswort & 1;
	}else{
		int zw=pow(2,15);
		error=statuswort & zw;
		
	}
	return error;
}

int check_spuelen(long zeitstempel){
	FILE *f=NULL;
	int wert=0;
	char file_spuelen_bin_pfad[]="file_spuelen.bin";
	f=fopen(file_spuelen_bin_pfad,"rb");
	if (f!=NULL){
		fread(&zeitstempel_spuelen, sizeof(long), 1, f);
		fclose(f);
	}else{
		char kommando[]="sudo touch file_spuelen.bin" ;
		f=popen(kommando,"r");
		fclose(f);
	}
	if(zeitstempel-zeitstempel_spuelen >= 24*3600){
		zeitstempel_spuelen=zeitstempel;
		f=fopen(file_spuelen_bin_pfad,"wb");
		fwrite(&zeitstempel_spuelen,sizeof(long),1,f);
		fclose(f)
		wert=1;
		zeitdiff_spuelen=zeitstempel;
	} else if(zeitstempel - zeitstempel_spuelen < TSPUELEN){
		wert=1;	
	}
	}else{
		wert=0;	
	}
	return wert;
}
void bit_order_write(uint16_t *werte, int order, int error_state, int current, long zeitstempel){
	int wert=0;
	int k,l, m;
	if (order==0){
			k=0;
			l=1;
			m=3;
	}else{
		k=15;
		l=14;
		m=12;
	}
	wert+=error_state*pow(2,k);
	if(current>0){
			wert+=pow(2,l);
	}
	if (check_spuelen(zeitstempel)==1){
		wert+=pow(2,m);	
	}	
	werte[0]=wert;
	
}


// args ip slave_id debug port
int main(int argc, char *argv[]) {
	srand(time(NULL));
	// nötige Elemente für Wartefunktion
		struct timespec tim, tim2, tim3, tim4;
		tim.tv_sec  = 10; // additon of seconds and nanoseconds, tim2 stores residual time if sleep is interrupted
		tim.tv_nsec = 0L;
		//tim.tv_nsec = 900000000L; //wartezeit vermutlich 10 s
	//
	char datum[100];
	//char pfad[100]="../EP_results/";
	long timestamp=get_time_string(datum,100); //EP_util include
	create_result_directory(pfad); //EP_util include
	//ctx_Pi = modbus_new_tcp("10.0.6.65",502);
	fprintf(stderr,"%s\n",argv[1]);
	//ctx_ELY=modbus_new_tcp(argv[1], atoi(argv[4]));
	ctx_ELY=modbus_new_tcp(argv[1],atoi(argv[4]));
	if(atoi(argv[2])!=0){
		modbus_set_slave(ctx_ELY,atoi(argv[2]));
	}
	modbus_connect(ctx_ELY);
	if(atoi(argv[3])!=0){
			modbus_set_debug(ctx_ELY,TRUE);
	}
	
	// write ELY file
	uint16_t zw_ELY[22];
     uint16_t *dest_ELY=&zw_ELY[0];
        char kommando_ELY[200]="sudo touch ELY.csv|mv ELY.csv " ;
		strcat(kommando_ELY, pfad);
        FILE *Datei1=popen(kommando_ELY, "r");
        pclose(Datei1);
        char pfad_ELY[100]="";
        strcat(pfad_ELY,pfad);
        strcat(pfad_ELY,"/ELY.csv");
        FILE *ELY_F=fopen(pfad_ELY, "a");
	char ELY_header[READ_END - READ_START+3][30]={"Zeitstempel[s]","Zeitstempel[1/10s]","EL_StatusWord1","EL_StatusWord2", "EL_Power_Act","EL_Current_Act", "EL_H2_Pressure_Act", "EL_Conductance_Act","EL_Temp_In_Act", "EL_EBM_FWKS_PropVentil", "EL_CalcH2Flow_Act","EL_CalcH2Volume_Sum" ,"EL_ELM1_Temp_Out_Act","EL_ELM2_Temp_Out_Act","EL_ELM3_Temp_Out_Act","EL_ELM4_Temp_Out_Act","EL_ELM5_Temp_Out_Act","EL_H2_Kuehler_Temp_Act", "EL_ErrorWord01","EL_ErrorWord02","EL_ErrorWord03", "EL_ErrorWord04", "EL_ErrorWord05", "EL_ConfigWord_GetValue"};
        for(int i=0; i<READ_END - READ_START+2;i++){
		fprintf(ELY_F, "%s,", ELY_header[i]);
	}
	fprintf(ELY_F,"%s\n",ELY_header[READ_END - READ_START+2]);
	fclose(ELY_F);
	long zeitpuffer[2];
	char *pfad_exp="./exp_plan.csv";
	char *pfad_snap="./snapshot";
	int t_step;
	int current_actual;
	int error_counter=0;
	FILE *fp;
	char str[60];
	char *first_line="";
	static int anzahl_regs=READ_END - READ_START;
	uint16_t zw[anzahl_regs];
	uint16_t *dest=&zw[0];
	uint16_t zw_write[5]={0};
	uint16_t *dest_write=&zw_write[0];
   /* opening file for reading */
   fp = fopen(pfad_exp , "r");
   if(fp == NULL) {
      perror("Error opening file");
      return(-1);
   }
	 if( fgets (str, 60, fp)!=NULL ) {
				first_line=strtok(str,",");
				t_step=atoi(first_line);
				fprintf(stderr,"%d",t_step);
				current_actual=atoi(strtok(NULL,","));
		}
		else{
			sigfunc(0);
		}
	clock_gettime(CLOCK_REALTIME, &tim4);
	for(;;){
		check_signal();
		clock_gettime(CLOCK_REALTIME, &tim3);
		zeitpuffer[0]=(long) tim3.tv_sec;
		zeitpuffer[1]=floor(tim3.tv_nsec / 1.0e8); // Zehntelsekunden
		if(t_step<(tim3.tv_sec-tim4.tv_sec)){
		
		 if( fgets (str, 60, fp)!=NULL ) {
			first_line=strtok(str,",");
			t_step=atoi(first_line);
			current_actual=atoi(strtok(NULL,","));
		} else{
			sigfunc(0);
		}
		}
		// run ELY read & write
		fprintf(stderr,"\nStartadresse: %d\n",READ_START-atoi(argv[5]));
		fprintf(stderr,"READ: \n%d",modbus_read_registers(ctx_ELY, READ_START-atoi(argv[5]), READ_END-READ_START,dest));
		
		if (atoi(argv[6])==1){
			change_byte_order(dest,anzahl_regs);
		}
		int j;
		FILE *snapshot_data=fopen(pfad_snap,"w");
		for(j=0;j<READ_END - READ_START;j++){
				fprintf(snapshot_data,"Register %d:%d\n",READ_START+j, dest[j]);
		}
		fclose(snapshot_data);
		FILE *ELY_F=fopen(pfad_ELY, "a");
		fprintf(ELY_F,"%ld,%ld,",zeitpuffer[0], zeitpuffer[1]);
		for(j=0;j<READ_END - READ_START-1;j++){
				fprintf(ELY_F,"%d,",dest[j]);
		}
		fprintf(ELY_F,"%d\n",dest[READ_END - READ_START-1]);
		fclose(ELY_F);
		int error=read_status_word1(dest[0], atoi(argv[7]));
		fprintf(stderr, "\nError:%d\n", error);
		if (error !=0){
			error_counter+=1;
		}else{
			error_counter=0;
		}
		if (error_counter > 5){
			sigfunc(0);
		}
		
		bit_order_write(dest_write,atoi(argv[7]),error,current_actual, zeitpuffer[0]);
		dest_write[2]=current_actual;
		if (current_actual > MAX_CURRENT){
			current_actual=MAX_CURRENT;
		}else if (current_actual < MIN_CURRENT){
			current_actual=0;
		}
		if (atoi(argv[6])==1){
			change_byte_order(dest_write,5);
		}
		modbus_write_registers(ctx_ELY, WRITE_START-atoi(argv[5]), 3, dest_write);
		nanosleep(&tim, &tim2);
	}
	
	
	fclose(fp);
}


