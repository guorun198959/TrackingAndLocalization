#include "ukf.h"
#include "Eigen/Dense"
#include <iostream>

using namespace std;
using Eigen::MatrixXd;
using Eigen::VectorXd;
using std::vector;

/**
 * Initializes Unscented Kalman filter
 */
UKF::UKF() {
  // if this is false, laser measurements will be ignored (except during init)
  use_laser = true;

  // if this is false, radar measurements will be ignored (except during init)
  use_radar = true;

  // initial state vector
  x = VectorXd(5);

  // initial covariance matrix
  P = MatrixXd(5, 5);

  // Process noise standard deviation longitudinal acceleration in m/s^2
  std_a = 2;

  // Process noise standard deviation yaw acceleration in rad/s^2
  std_yawdd = 3.14/12;


  //Don't change these----------------------
  // Laser measurement noise standard deviation position1 in m
  std_laspx = 0.15;

  // Laser measurement noise standard deviation position2 in m
  std_laspy = 0.15;

  // Radar measurement noise standard deviation radius in m
  std_radr = 0.3;

  // Radar measurement noise standard deviation angle in rad
  std_radphi = 0.03;

  // Radar measurement noise standard deviation radius change in m/s
  std_radrd = 0.3;

  /**
  TODO:

  Complete the initialization. See ukf.h for other member properties.

  Hint: one or more values initialized above might be wildly off...
  */
  is_initialized = false;
  n_x = 5;
  n_aug = 7;
  lambda = 3 - n_aug;
}

UKF::~UKF() {}

/**
 * @param {MeasurementPackage} meas_package The latest measurement data of
 * either radar or laser.
 */
void UKF::ProcessMeasurement(MeasurementPackage measurement_pack) {
  /**
  TODO:

  Complete this function! Make sure you switch between lidar and radar
  measurements.
  */
	if(!is_initialized){
		 if (measurement_pack.sensor_type_ == MeasurementPackage::RADAR) {
				float rho = measurement_pack.raw_measurements_[0];
				float theta = measurement_pack.raw_measurements_[1];
				x << rho*cos(theta),rho*sin(theta), 0, 0, 0;    
				P << pow(std_radr*std_radphi,2),0,0,0,0,
					0,pow(std_radr*std_radphi,2),0,0,0,
					0,0,1,0,0,
					0,0,0,1,0,
					0,0,0,0,1;
			}
			else if (measurement_pack.sensor_type_ == MeasurementPackage::LASER){ 
				x << measurement_pack.raw_measurements_[0], measurement_pack.raw_measurements_[1], 0, 0, 0;	    	    	
				P << std_laspx*std_laspx,0,0,0,0,
					0,std_laspy*std_laspy,0,0,0,
					0,0,1,0,0,
					0,0,0,1,0,
					0,0,0,0,1;
			}
			
			previous_timestamp_ = measurement_pack.timestamp_;
			// done initializing, no need to predict or update
			is_initialized = true;	
			std::cout<<"init done-------"<<endl;
			return;
	}
	if (use_radar==true && measurement_pack.sensor_type_ == MeasurementPackage::RADAR) {
		double dt = (measurement_pack.timestamp_ - previous_timestamp_) / 1000000.0;	//dt - expressed in seconds
		previous_timestamp_ = measurement_pack.timestamp_;
		std::cout<<"predictimng using Radar"<<endl;
		Prediction(dt);
		std::cout<<"Done predict Radar"<<endl;
		
		std::cout<<"updatingusing Radar"<<endl;
		UpdateRadar(measurement_pack);
		std::cout<<"DOne update using Radar"<<endl;
			
	}
	
	if (use_laser==true && measurement_pack.sensor_type_ == MeasurementPackage::LASER) {
			double dt = (measurement_pack.timestamp_ - previous_timestamp_) / 1000000.0;	//dt - expressed in seconds
			previous_timestamp_ = measurement_pack.timestamp_;
			std::cout<<"predictimng using LIDAR"<<endl;
			Prediction(dt);
			std::cout<<"Done predict LIDARr"<<endl;
			std::cout<<"updatingusing LIDAR"<<endl;
			UpdateLidar(measurement_pack);
			std::cout<<"DOne update using LIDAR"<<endl;
		}
}

/**
 * Predicts sigma points, the state, and the state covariance matrix.
 * @param {double} delta_t the change in time (in seconds) between the last
 * measurement and this one.
 */
void UKF::Prediction(double delta_t) {
  /**
  TODO:

  Complete this function! Estimate the object's location. Modify the state
  vector, x_. Predict sigma points, the state, and the state covariance matrix.
  */
	
	  
	  
	  //---------------------------------------------------------------------------
	  //create augmented mean vector
	   VectorXd x_aug = VectorXd(7);

	   //create augmented state covariance
	   MatrixXd P_aug = MatrixXd(7, 7);

	   //create sigma point matrix
	   MatrixXd Xsig_aug = MatrixXd(n_aug, 2 * n_aug + 1);

	   //create augmented mean state
	   x_aug.head(5) = x;
	   x_aug(5) = 0;
	   x_aug(6) = 0;

	   //create augmented covariance matrix
	   P_aug.fill(0.0);
	   P_aug.topLeftCorner(5,5) = P;
	   P_aug(5,5) = std_a*std_a;
	   P_aug(6,6) = std_yawdd*std_yawdd;

	   //create square root matrix
	   MatrixXd L = P_aug.llt().matrixL();

	   //create augmented sigma points
	   Xsig_aug.col(0)  = x_aug;
	   for (int i = 0; i< n_aug; i++)
	   {
	     Xsig_aug.col(i+1)       = x_aug + sqrt(lambda+n_aug) * L.col(i);
	     Xsig_aug.col(i+1+n_aug) = x_aug - sqrt(lambda+n_aug) * L.col(i);
	   }
	  //=====================================================================
	
	   
	   //-----------------------------------------------------------
	   //create matrix with predicted sigma points as columns
	   // Xsig_pred BELOW IS THERE AS CLASS. tHESE will also be used for measuremnet update
	      Xsig_pred = MatrixXd(n_x, 2 * n_aug + 1);

//	     double delta_t = 0.1; //time diff in sec


	     //predict sigma points
	     for (int i = 0; i< 2*n_aug+1; i++)
	     {
	       //extract values for better readability
	       double p_x = Xsig_aug(0,i);
	       double p_y = Xsig_aug(1,i);
	       double v = Xsig_aug(2,i);
	       double yaw = Xsig_aug(3,i);
	       double yawd = Xsig_aug(4,i);
	       double nu_a = Xsig_aug(5,i);
	       double nu_yawdd = Xsig_aug(6,i);

	       //predicted state values
	       double px_p, py_p;

	       //avoid division by zero
	       if (fabs(yawd) > 0.001) {
	           px_p = p_x + v/yawd * ( sin (yaw + yawd*delta_t) - sin(yaw));
	           py_p = p_y + v/yawd * ( cos(yaw) - cos(yaw+yawd*delta_t) );
	       }
	       else {
	           px_p = p_x + v*delta_t*cos(yaw);
	           py_p = p_y + v*delta_t*sin(yaw);
	       }

	       double v_p = v;
	       double yaw_p = yaw + yawd*delta_t;
	       double yawd_p = yawd;

	       //add noise
	       px_p = px_p + 0.5*nu_a*delta_t*delta_t * cos(yaw);
	       py_p = py_p + 0.5*nu_a*delta_t*delta_t * sin(yaw);
	       v_p = v_p + nu_a*delta_t;

	       yaw_p = yaw_p + 0.5*nu_yawdd*delta_t*delta_t;
	       yawd_p = yawd_p + nu_yawdd*delta_t;

	       //write predicted sigma point into right column
	       Xsig_pred(0,i) = px_p;
	       Xsig_pred(1,i) = py_p;
	       Xsig_pred(2,i) = v_p;
	       Xsig_pred(3,i) = yaw_p;
	       Xsig_pred(4,i) = yawd_p;
	     }
	     //--------------------------------------------------------------------
		 
	     //========================================================================
	     // set weights
		  weights = VectorXd(2*n_aug+1);
	       double weight_0 = lambda/(lambda+n_aug);
	       weights(0) = weight_0;
	       for (int i=1; i<2*n_aug+1; i++) {  //2n+1 weights
	         double weight = 0.5/(n_aug+lambda);
	         weights(i) = weight;
	       }

	       //predicted state mean
	       x.fill(0.0);
	       for (int i = 0; i < 2 * n_aug + 1; i++) {  //iterate over sigma points
	         x = x+ weights(i) * Xsig_pred.col(i);
	       }

	       //predicted state covariance matrix
	       P.fill(0.0);
	       for (int i = 0; i < 2 * n_aug + 1; i++) {  //iterate over sigma points

	         // state difference
	         VectorXd x_diff = Xsig_pred.col(i) - x;
	         //angle normalization
	         while (x_diff(3)> M_PI) x_diff(3)-=2.*M_PI;
	         while (x_diff(3)<-M_PI) x_diff(3)+=2.*M_PI;

	         P = P + weights(i) * x_diff * x_diff.transpose() ;
	       }
	 	 
	
}

/**
 * Updates the state and the state covariance matrix using a laser measurement.
 * @param {MeasurementPackage} meas_package
 */
void UKF::UpdateLidar(MeasurementPackage meas_package) {
  /**
  TODO:

  Complete this function! Use lidar data to update the belief about the object's
  position. Modify the state vector, x_, and covariance, P_.

  You'll also need to calculate the lidar NIS.
  */
	
		MatrixXd R_ = MatrixXd(2, 2);
		R_ << std_laspx*std_laspx, 0,
		        0, std_laspy*std_laspy;
		
		MatrixXd H_ = MatrixXd(2, 5);
		H_<< 1,0,0,0,0,
			0,1,0,0,0;
		
		 VectorXd z = meas_package.raw_measurements_;
				
		VectorXd z_pred = H_ * x;
		VectorXd y = z - z_pred;
		MatrixXd Ht = H_.transpose();
		MatrixXd S = H_ * P * Ht + R_;
		MatrixXd Si = S.inverse();
		MatrixXd PHt = P * Ht;
		MatrixXd K = PHt * Si;
		
		//new estimate
		x = x + (K * y);
		long x_size = x.size();
		MatrixXd I = MatrixXd::Identity(x_size, x_size);
		P = (I - K * H_) * P;
	
}

/**
 * Updates the state and the state covariance matrix using a radar measurement.
 * @param {MeasurementPackage} meas_package
 */
void UKF::UpdateRadar(MeasurementPackage meas_package) {
  /**
  TODO:

  Complete this function! Use radar data to update the belief about the object's
  position. Modify the state vector, x_, and covariance, P_.

  You'll also need to calculate the radar NIS.
  */
	
	//--------------------------------------------------------
	 //set state dimension
	  int n_x = 5;

	  //set augmented dimension
	  int n_aug = 7;

	  //set measurement dimension, radar can measure r, phi, and r_dot
	  int n_z = 3;

	  //define spreading parameter
	  double lambda = 3 - n_aug;

	  //set vector for weights
	   weights = VectorXd(2*n_aug+1);
	   double weight_0 = lambda/(lambda+n_aug);
	  weights(0) = weight_0;
	  for (int i=1; i<2*n_aug+1; i++) {  
	    double weight = 0.5/(n_aug+lambda);
	    weights(i) = weight;
	  }
	  
	  //create matrix for sigma points in measurement space
	    MatrixXd Zsig = MatrixXd(n_z, 2 * n_aug + 1);


	    //transform sigma points into measurement space
	    for (int i = 0; i < 2 * n_aug + 1; i++) {  //2n+1 simga points

	      // extract values for better readibility
	      double p_x = Xsig_pred(0,i);
	      double p_y = Xsig_pred(1,i);
	      double v  = Xsig_pred(2,i);
	      double yaw = Xsig_pred(3,i);

	      double v1 = cos(yaw)*v;
	      double v2 = sin(yaw)*v;

	      // measurement model
	      Zsig(0,i) = sqrt(p_x*p_x + p_y*p_y);                        //r
	      Zsig(1,i) = atan2(p_y,p_x);                                 //phi
	      Zsig(2,i) = (p_x*v1 + p_y*v2 ) / sqrt(p_x*p_x + p_y*p_y);   //r_dot
	    }

	    //mean predicted measurement
	    VectorXd z_pred = VectorXd(n_z);
	    z_pred.fill(0.0);
	    for (int i=0; i < 2*n_aug+1; i++) {
	        z_pred = z_pred + weights(i) * Zsig.col(i);
	    }

	    //measurement covariance matrix S
	    MatrixXd S = MatrixXd(n_z,n_z);
	    S.fill(0.0);
	    for (int i = 0; i < 2 * n_aug + 1; i++) {  //2n+1 simga points
	      //residual
	      VectorXd z_diff = Zsig.col(i) - z_pred;

	      //angle normalization
	      while (z_diff(1)> M_PI) z_diff(1)-=2.*M_PI;
	      while (z_diff(1)<-M_PI) z_diff(1)+=2.*M_PI;

	      S = S + weights(i) * z_diff * z_diff.transpose();
	    }

	    //add measurement noise covariance matrix
	    MatrixXd R = MatrixXd(n_z,n_z);
	    R <<    std_radr*std_radr, 0, 0,
	            0, std_radphi*std_radphi, 0,
	            0, 0,std_radrd*std_radrd;
	    S = S + R;
	  
	//=========================================================
	    
	//------------------------------------------------------
	    //create matrix for cross correlation Tc
	    MatrixXd Tc = MatrixXd(n_x, n_z);

	    //calculate cross correlation matrix
	    Tc.fill(0.0);
	    for (int i = 0; i < 2 * n_aug + 1; i++) {  //2n+1 simga points

	      //residual
	      VectorXd z_diff = Zsig.col(i) - z_pred;
	      //angle normalization
	      while (z_diff(1)> M_PI) z_diff(1)-=2.*M_PI;
	      while (z_diff(1)<-M_PI) z_diff(1)+=2.*M_PI;

	      // state difference
	      VectorXd x_diff = Xsig_pred.col(i) - x;
	      //angle normalization
	      while (x_diff(3)> M_PI) x_diff(3)-=2.*M_PI;
	      while (x_diff(3)<-M_PI) x_diff(3)+=2.*M_PI;

	      Tc = Tc + weights(i) * x_diff * z_diff.transpose();
	    }

	    //Kalman gain K;
	    MatrixXd K = Tc * S.inverse();
	    VectorXd z = meas_package.raw_measurements_;
	    //residual
	    VectorXd z_diff = z - z_pred;

	    //angle normalization
	    while (z_diff(1)> M_PI) z_diff(1)-=2.*M_PI;
	    while (z_diff(1)<-M_PI) z_diff(1)+=2.*M_PI;

	    //update state mean and covariance matrix
	    x = x + K * z_diff;
	    P = P - K*S*K.transpose();
	//======================================================    
	
}
