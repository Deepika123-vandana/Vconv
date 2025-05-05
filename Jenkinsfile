pipeline {
    agent any

    environment {
        GIT_REPO_URL = 'https://github.com/Deepika123-vandana/Vconv.git'
        BRANCH = 'main'
        BUILD_DIR = 'build'
        SYSTEMC_HOME = '/home/admin1/Pictures/systemc'
        CPLUS_INCLUDE_PATH = "${SYSTEMC_HOME}/include:$CPLUS_INCLUDE_PATH"  // Adding systemc include path
        LD_LIBRARY_PATH = "${SYSTEMC_HOME}/lib:$LD_LIBRARY_PATH"  // Adding systemc library path
    }

    stages {
        stage('Checkout') {
            steps {
                git branch: "${BRANCH}", url: "${GIT_REPO_URL}"
            }
        }

        stage('Build') {
            steps {
                sh '''
                    mkdir -p ${BUILD_DIR}
                    g++ -I${SYSTEMC_HOME}/include -L${SYSTEMC_HOME}/lib -lsystemc conv_withtimings.cpp -o ${BUILD_DIR}/vconv.exe
                '''
            }
        }

        stage('Run') {
            steps {
                sh '''
                    ./${BUILD_DIR}/vconv.exe
                '''
            }
        }
    }
}
