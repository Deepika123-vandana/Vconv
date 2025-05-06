pipeline {
    agent any

    environment {
        GIT_REPO_URL = 'https://github.com/Deepika123-vandana/Vconv.git'
        BRANCH = 'main'
        BUILD_DIR = 'build'
        SYSTEMC_HOME = '/home/admin1/Documents/systemc/install'
        SYSTEMC_ROOT = '/home/admin1/Documents/systemc'  // Add SYSTEMC_ROOT for root folder
        LOG_DIR = '/home/admin1/Documents/systemc/jenkins' // Directory to move log.txt
    }

    triggers {
        githubPush()
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
                    sh '''
                        export LD_LIBRARY_PATH=$SYSTEMC_HOME/lib:$LD_LIBRARY_PATH
                        ./${BUILD_DIR}/vconv.exe
                        
                        # Ensure log.txt is generated in the correct directory
                        if [ -f /home/admin1/Documents/systemc/log.txt ]; then
                            mv /home/admin1/Documents/systemc/log.txt $LOG_DIR/log.txt
                        else
                            echo "log.txt not found"
                        fi

                        # Verify log.txt has been moved to the Jenkins directory
                        ls -l $LOG_DIR
                        [ -f $LOG_DIR/log.txt ] && cat $LOG_DIR/log.txt || echo "log.txt not found"
                    '''
                }
            }
        }

        stage('Archive Log') {
            steps {
                archiveArtifacts artifacts: 'systemc/jenkins/log.txt', allowEmptyArchive: true
            }
        }
    }

    post {
        failure {
            emailext(
                subject: "Jenkins Build Failed: ${env.JOB_NAME} [#${env.BUILD_NUMBER}]",
                body: """
                    Jenkins build failed.

                    Job: ${env.JOB_NAME}
                    Build: #${env.BUILD_NUMBER}
                    Commit: ${env.GIT_COMMIT}
                    View logs: ${env.BUILD_URL}console
                """,
                recipientProviders: [[$class: 'DevelopersRecipientProvider']]
            )
        }
    }
}
