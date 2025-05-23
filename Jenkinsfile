pipeline {
    agent any

    environment {
        GIT_REPO_URL = 'https://github.com/Deepika123-vandana/Vconv.git'
        BRANCH = 'main'
        BUILD_DIR = 'build'
        SYSTEMC_HOME = '/home/admin1/Documents/systemc/install'
        LOG_DIR = '/home/admin1/Documents/systemc/jenkins'
        TEAM_LEAD_EMAIL = 'deepikavandana41@gmail.com'
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

        stage('Sanity Check - HelloWorld') {
            steps {
                script {
                    sh '''
                        export CPLUS_INCLUDE_PATH=$SYSTEMC_HOME/include:$CPLUS_INCLUDE_PATH
                        export LD_LIBRARY_PATH=$SYSTEMC_HOME/lib:$LD_LIBRARY_PATH
                        mkdir -p ${BUILD_DIR}
                        g++ -I$SYSTEMC_HOME/include -L$SYSTEMC_HOME/lib -lsystemc helloworld.cpp -o ${BUILD_DIR}/helloworld.exe

                        echo "Running HelloWorld sanity check..."
                        ./${BUILD_DIR}/helloworld.exe
                    '''
                }
            }
        }

        stage('Build Main File') {
            steps {
                script {
                    sh '''
                        export CPLUS_INCLUDE_PATH=$SYSTEMC_HOME/include:$CPLUS_INCLUDE_PATH
                        export LD_LIBRARY_PATH=$SYSTEMC_HOME/lib:$LD_LIBRARY_PATH
                        g++ -I$SYSTEMC_HOME/include -L$SYSTEMC_HOME/lib -lsystemc conv_withtimings.cpp -o ${BUILD_DIR}/vconv.exe
                    '''
                }
            }
        }

        stage('Run Main Application') {
            steps {
                script {
                    sh '''
                        export LD_LIBRARY_PATH=$SYSTEMC_HOME/lib:$LD_LIBRARY_PATH
                        ./${BUILD_DIR}/vconv.exe

                        # Ensure log directory exists
                        mkdir -p $LOG_DIR

                        # Move the log.txt if exists
                        if [ -f /home/admin1/Documents/systemc/log.txt ]; then
                            mv /home/admin1/Documents/systemc/log.txt $LOG_DIR/log.txt
                        else
                            echo "log.txt not found"
                        fi

                        # Show the moved log file
                        ls -l $LOG_DIR
                        [ -f $LOG_DIR/log.txt ] && cat $LOG_DIR/log.txt || echo "log.txt not found"
                    '''
                }
            }
        }
    }

    post {
        success {
            script {
                def commitAuthor = sh(script: "git log -1 --pretty=format:'%an <%ae>'", returnStdout: true).trim()
                emailext(
                    subject: "Jenkins Build Succeeded: ${env.JOB_NAME} [#${env.BUILD_NUMBER}]",
                    body: """
                        Jenkins build completed successfully.

                        Job: ${env.JOB_NAME}
                        Build: #${env.BUILD_NUMBER}
                        Commit: ${env.GIT_COMMIT}
                        Author: ${commitAuthor}

                        View console log: ${env.BUILD_URL}console

                        -- Jenkins
                    """,
                    to: "${commitAuthor}, ${TEAM_LEAD_EMAIL}"
                )
            }
        }

        failure {
            emailext(
                subject: "Jenkins Build Failed: ${env.JOB_NAME} [#${env.BUILD_NUMBER}]",
                body: """
                    Jenkins build failed.

                    Job: ${env.JOB_NAME}
                    Build: #${env.BUILD_NUMBER}
                    Commit: ${env.GIT_COMMIT}
                    View logs: ${env.BUILD_URL}console

                    -- Jenkins
                """,
                recipientProviders: [[$class: 'DevelopersRecipientProvider']]
            )
        }
    }
}
