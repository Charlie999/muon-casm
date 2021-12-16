
node("windows-1") {
  stage("Build CASM for Windows") {
    git url: 'https://github.com/MUON-III/muon-casm.git'
    dir("build"){
      bat 'cmake .. -G "Unix Makefiles"'
      bat 'make'
      archiveArtifacts artifacts: 'casm.exe', fingerprint: true
      deleteDir()
    }
  }
}
node("master") {
  stage("Build CASM-STATIC for Linux") {
    git url: 'https://github.com/MUON-III/muon-casm.git'
    dir("build"){
      sh 'cmake .. -DVERSION=latest -DBUILD_STATIC=true'
      sh 'make'
      archiveArtifacts artifacts: 'casm-static*', fingerprint: true
      deleteDir()
    }
  }
}
