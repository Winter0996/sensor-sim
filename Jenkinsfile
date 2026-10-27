pipeline {
    agent any

    stages {
        stage('Configure') {
            steps {
                sh 'cmake -S . -B build'
            }
        }

        stage('Build') {
            steps {
                sh 'cmake --build build'
            }
        }

        stage('Test') {
            steps {
                sh 'ctest --test-dir build --output-on-failure'
            }
        }

        stage('Static analysis') {
            parallel {
                stage('cppcheck') {
                    steps {
                        sh 'cppcheck --enable=warning,style,performance,portability --std=c++17 --error-exitcode=1 --suppress=missingIncludeSystem -I src src/'
                    }
                }
                stage('clang-tidy') {
                    steps {
                        sh 'clang-tidy -p build src/parser.cpp src/sequence_tracker.cpp src/sender.cpp src/receiver.cpp'
                    }
                }
            }
        }
    }

    post {
        always {
            echo 'Pipeline finished'
        }
    }
}