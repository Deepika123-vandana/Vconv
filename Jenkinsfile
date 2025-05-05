pipeline {
    agent any

    environment {
        GIT_REPO_URL = 'https://github.com/Deepika123-vandana/Vconv.git'
        BRANCH = 'main'
        BUILD_DIR = 'build'
        SYSTEMC_HOME = '/home/admin1/Pictures/systemc/install' // Correct path to the SystemC installation
    }

    stages {
        stage('Checkout') {
            steps {
                git branch: "${BRANCH}", url: "${GIT_REPO_URL}"
            }
        }

        stage('Build') {
            steps {
                script {
                    // Ensure the SystemC include and lib paths are properly exported
                    sh '''
                        export CPLUS_INCLUDE_PATH=$SYSTEMC_HOME/include:$CPLUS_INCLUDE_PATH
                        export LD_LIBRARY_PATH=$SYSTEMC_HOME/lib:$LD_LIBRARY_PATH
                        mkdir -p ${BUILD_DIR}
                        g++ -I$SYSTEMC_HOME/include -L$SYSTEMC_HOME/lib -lsystemc conv_withtimings.cpp -o ${BUILD_DIR}/vconv.exe
                    '''
                }
            }
        }

        stage('Run') {
            steps {
                script {
                    // Ensure libraries are linked before running
                    sh '''
                        export LD_LIBRARY_PATH=$SYSTEMC_HOME/lib:$LD_LIBRARY_PATH
                        ./${BUILD_DIR}/vconv.exe
                    '''
                }
            }
        }
    }
}
