node("windows-1") {
  stage("Build CASM for Windows") {
    dir("build"){
      bat 'cd build && cmake .. -G "Unix Makefiles"'
      bat 'cd build && make'
      deleteDir()
    }
  }
}
node("master") {
  stage("Build CASM-STATIC for Linux") {
    dir("build"){
      sh 'cd build && cmake .. -DUSE_GIT_VERSION=true -DBUILD_STATIC=true'
      sh 'cd build && make'
      deleteDir()
    }
  }
}

post {
    always {
      node("windows-1"){
        archiveArtifacts artifacts: 'build/casm.exe', fingerprint: true
      }
      node("master"){
        archiveArtifacts artifacts: 'build/casm-static*', fingerprint: true
      }
    }
    cleanup { 
        cleanWs() 
    }
  }
