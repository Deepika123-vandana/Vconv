pipeline {
    agent any

    environment {
        GIT_REPO_URL = 'https://github.com/Deepika123-vandana/Vconv.git'
        BRANCH = 'main'
        BUILD_DIR = 'build'
        SYSTEMC_HOME = 'SYSTEMC_HOME = '/home/admin1/Pictures/systemc'' 
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
                    g++ -I$SYSTEMC_HOME/include -L$SYSTEMC_HOME/lib -lsystemc conv_withtimings.cpp -o ${BUILD_DIR}/vconv.exe
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
